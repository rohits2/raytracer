#include "mesh.h"

float BoundingBox::volume() {
    Vec3 edges = ll - ur;
    return fabs(edges.x * edges.y * edges.z);
}

bool BoundingBox::contains(Vec3 point) const {
    bool ll_sat = point.x >= ll.x && point.y >= ll.y && point.z >= ll.z;
    bool ur_sat = point.x < ur.x && point.y < ur.y && point.z < ur.z;
    return ll_sat && ur_sat;
}

Mesh::Mesh(vector<Vec3> vertices, vector<Face> faces, vector<Vec3> colors, float ior, float diffusion, float smoothness) {
    this->vertices = vertices;
    this->faces = faces;
    this->colors = colors;
    this->ior = ior;
    this->diffusion = diffusion;
    this->smoothness = smoothness;
}

Vec3 parse_vec3(char **line_start) {
    char *start = *line_start;
    char *end = *line_start;
    while (*end != '\n') {
        end++;
    }
    *end = '\0';
    *line_start = end;
    char *strtok_save;
    char *v = strtok_r(start, " ", &strtok_save);
    char *x = strtok_r(nullptr, " ", &strtok_save);
    char *y = strtok_r(nullptr, " ", &strtok_save);
    char *z = strtok_r(nullptr, " ", &strtok_save);
    if (*v == 'v' && x != nullptr && y != nullptr && z != nullptr) {
        *end = '\n';
        return Vec3(atof(x), atof(y), atof(z));
    }
    fprintf(stderr, "Could not parse Vec3! %s %s %s %s\n", v, x, y, z);
    exit(-2);
}

Face parse_face(char **line_start) {
    char *start = *line_start;
    char *end = *line_start;
    while (*end != '\n') {
        end++;
    }
    *end = '\0';
    *line_start = end;
    char *strtok_save;
    char *f = strtok_r(start, " ", &strtok_save);
    char *x = strtok_r(nullptr, " ", &strtok_save);
    char *y = strtok_r(nullptr, " ", &strtok_save);
    char *z = strtok_r(nullptr, " ", &strtok_save);
    if (*f == 'f') {
        *end = '\n';
        return Face(atol(x) - 1, atol(y) - 1, atol(z) - 1, -1, -1);
    }
    fprintf(stderr, "Could not parse Face! %s %s %s %s\n", f, x, y, z);
    exit(-2);
}

void Mesh::init_rtree() {
    float min_x = INFINITY, min_y = INFINITY, min_z = INFINITY;
    float max_x = -INFINITY, max_y = -INFINITY, max_z = -INFINITY;
    for (Vec3 point : vertices) {
        min_x = fmin(point.x, min_x);
        min_y = fmin(point.y, min_y);
        min_z = fmin(point.z, min_z);
        max_x = fmax(point.x + EPS, max_x);
        max_y = fmax(point.y + EPS, max_y);
        max_z = fmax(point.z + EPS, max_z);
    }
    BoundingBox bbox(Vec3(min_x, min_y, min_z), Vec3(max_x, max_y, max_z));
    root.bbox = bbox;
}

Mesh::Mesh(char *obj_file) {
    // Get file size
    struct stat st;
    int stat_result = stat(obj_file, &st);
    if (stat_result == -1) {
        fprintf(stderr, "Could not stat file %s!\n", obj_file);
        exit(-1);
    }
    size_t size = st.st_size;
    // Open fd
    int fd = open(obj_file, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Could not open file %s!\n", obj_file);
        exit(-1);
    }

    // mmap file
    char *file = (char *)mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

    // Parse until seek head is past end of file
    int object_count = 0;
    for (char *seek = file; seek < file + size; seek++) {
        switch (*seek) {
        case ' ': {
            continue;
        }
        case '\n': {
            continue;
        }
        case 'o': {
            if (object_count >= 1) {
                fprintf(stderr, "Too many objects in %s!\n", obj_file);
                exit(-2);
            }
            object_count++;
            break;
        }
        case 'v': {
            Vec3 vec = parse_vec3(&seek);
#ifdef DEBUG_PARSE
            printf("v (%f, %f, %f)\n", vec.x, vec.y, vec.z);
#endif
            vertices.push_back(vec);
            break;
        }
        case 'f': {
            Face face = parse_face(&seek);
#ifdef DEBUG_PARSE
            printf("f (%d, %d, %d)/(%d, %d)\n", face.v0, face.v1, face.v2, face.normal, face.c);
#endif
            faces.push_back(face);
            break;
        }
        case 's': {
            fprintf(stderr, "Smooth shading controls not yet supported! File %s.\n", obj_file);
            exit(-2);
            break;
        }
        default: {
            fprintf(stdout, "Unknown error in %s! Char %c was encountered unexpectedly.\n", obj_file, *seek);
            exit(-2);
        }
        }
    }
    // Unmap file
    munmap(file, size);
    close(fd);

    // Build normals
    printf("Parsed %zu vertices and %zu faces from %s!\n", vertices.size(), faces.size(), obj_file);
    for (Face &face : faces) {
        Vec3 l = vertices[face.v0] - vertices[face.v1]; // v0-v1
        Vec3 r = vertices[face.v2] - vertices[face.v1]; // v2-v1
        normals.push_back(l % r);
        face.normal = normals.size() - 1;
    }

    // Make R-Tree
    init_rtree();
    for (int i = 0; i < faces.size(); i++) {
        insert_face(i);
    }
}

float median(vector<float> data) {
    sort(data.begin(), data.end());
    float firstVal = data[0];
    for (int i = data.size() / 2; i < data.size(); i++) {
        if (data[i] != firstVal) {
            return data[i];
        }
    }
    fprintf(stderr, "Could not split array into 50th percentile! Check the integrity of the OBJ file.\n");
    return firstVal+EPS;
}

void Mesh::split_node(RTreeNode *node) {
    // Get all XYZ of owned vertices
    vector<float> xs, ys, zs;
    for (int face_id : node->incident_faces) {
        Face &sface = faces[face_id];
        auto sverts = {vertices[sface.v0], vertices[sface.v1], vertices[sface.v2]};
        for (const Vec3 &svert : sverts) {
            if (node->bbox.contains(svert)) {
                xs.push_back(svert.x);
                ys.push_back(svert.y);
                zs.push_back(svert.z);
            }
        }
    }
    // Find the median in all dimensions
    float mx = median(xs);
    float my = median(ys);
    float mz = median(zs);
    if (xs[0] == xs[xs.size() - 1] && ys[0] == ys[ys.size() - 1] && zs[0] == zs[zs.size() - 1]) {
#ifdef DEBUG_RTREE
        printf("Cannot split this node - all vertices are coincident!\n");
#endif
        return;
    }

    // Find the center of the axis
    Vec3 center = (node->bbox.ur + node->bbox.ll) / 2;
    // Find the most even split in both geometric and dense terms
    int split_dim = -1;
    float split_value = NAN;
    float best_diff = INFINITY;
    if (fabs(mx - center.x) < best_diff && xs[0] != xs[xs.size() - 1]) {
        split_dim = 1;
        split_value = mx;
        best_diff = fabs(mx - center.x);
    }
    if (fabs(my - center.y) < best_diff && ys[0] != ys[ys.size() - 1]) {
        split_dim = 1;
        split_value = my;
        best_diff = fabs(my - center.y);
    }
    if (fabs(mz - center.z) < best_diff && zs[0] != zs[zs.size() - 1]) {
        split_dim = 2;
        split_value = mz;
        best_diff = fabs(mz - center.z);
    }
#ifdef DEBUG_RTREE
    printf("Splitting on axis %d, divergence %f\n", split_dim, best_diff);
#endif

    // Erase the current node in preparation for the new one
    set<int> old_faces = node->incident_faces;
    node->incident_faces.clear();

    // Calculate the new boundary after the split
    RTreeNode new_node;
    Vec3 new_brk_ll = node->bbox.ll;
    Vec3 new_brk_ur = node->bbox.ur;
    switch (split_dim) {
    case 0: {
        new_brk_ll.x = split_value;
        new_brk_ur.x = split_value;
        break;
    }
    case 1: {
        new_brk_ll.y = split_value;
        new_brk_ur.y = split_value;
        break;
    }
    case 2: {
        new_brk_ll.z = split_value;
        new_brk_ur.z = split_value;
        break;
    }
    }

    // Construct one new node
    new_node.bbox = BoundingBox(new_brk_ll, node->bbox.ur);
    // If it can be added as a peer leaf to the current node, do so
    if (node->parent != nullptr && node->parent->children.size() < RTREE_MAXIMUM_SUBDIVISIONS) {
        node->bbox = BoundingBox(node->bbox.ll, new_brk_ur);
        new_node.parent = node->parent;
        node->parent->children.push_back(new_node);
#ifdef DEBUG_RTREE
        printf("Adding new node as peer, parent now has %zu children\n", node->parent->children.size());
        printf("Re-inserting %zu faces into parent...\n", old_faces.size());
#endif
        for (int face_id : old_faces) {
            insert_face(face_id, node->parent);
        }
    } else {
#ifdef DEBUG_RTREE
        printf("Parent node is full/nonexistent, adding extra nodes to divide this one.\n");
#endif
        // Otherwise, create another node to make the current node internal
        RTreeNode new_node2;
        new_node2.bbox = BoundingBox(node->bbox.ll, new_brk_ur);
        new_node.parent = node;
        new_node2.parent = node;
        node->children.push_back(new_node);
        node->children.push_back(new_node2);
#ifdef DEBUG_RTREE
        printf("Re-inserting %zu faces into node...\n", old_faces.size());
#endif
        for (int face_id : old_faces) {
            insert_face(face_id, node);
            printf("Insert %d\n", face_id);
        }
    }
#ifdef DEBUG_RTREE

#endif
    // Re-insert all faces
}

void Mesh::insert_face(int face_i) {
    Face face = faces[face_i];
    auto verts = {vertices[face.v0], vertices[face.v1], vertices[face.v2]};
    for (const Vec3 &vert : verts) {
        // Find host node
        RTreeNode *cur_root = &root;
        while (cur_root->children.size() > 0) {
            for (RTreeNode &child : cur_root->children) {
                if (child.bbox.contains(vert)) {
                    cur_root = &child;
                    break;
                }
            }
        }
        // Push to node
        if (find(cur_root->incident_faces.begin(), cur_root->incident_faces.end(), face_i) == cur_root->incident_faces.end()) {
            cur_root->incident_faces.insert(face_i);
        }
        // Determine if the node requires a split
        if (cur_root->incident_faces.size() > RTREE_MAXIMUM_FACES) {
            split_node(cur_root);
        }
    }
}

void Mesh::insert_face(int face_i, RTreeNode *root) {
    Face face = faces[face_i];
    auto verts = {vertices[face.v0], vertices[face.v1], vertices[face.v2]};
    for (const Vec3 &vert : verts) {
        // Find host node
        RTreeNode *cur_root = root;
        RTreeNode *last_root = root;
        while (cur_root->children.size() > 0) {
            for (RTreeNode &child : cur_root->children) {
                if (child.bbox.contains(vert)) {
                    cur_root = &child;
                    break;
                }
            }
            if (cur_root == last_root) {
                printf("INFINITE LOOP DET.\n");
            }
        }
        // Push to node
        if (find(cur_root->incident_faces.begin(), cur_root->incident_faces.end(), face_i) == cur_root->incident_faces.end()) {
            cur_root->incident_faces.insert(face_i);
        }
        // Determine if the node requires a split
        if (cur_root->incident_faces.size() > RTREE_MAXIMUM_FACES) {
            split_node(cur_root);
        }
    }
}

RaycastResult Mesh::raycast(Vec3 origin, Vec3 direction) const {}