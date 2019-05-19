#include "mesh.h"

Mesh::Mesh(vector<Vec3> vertices, vector<Face> faces, vector<Vec3> colors, float ior, float matte, float shiny) {
    this->vertices = vertices;
    this->faces = faces;
    this->colors = colors;
    this->ior = ior;
    this->matte = matte;
    this->shiny = shiny;
}

void parse_comment(char **line_start) {
    char *end = *line_start;
    while (*end != '\n') {
        end++;
    }
    *line_start = end;
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

void Mesh::init_octree() {
    float min_x = INFINITY, min_y = INFINITY, min_z = INFINITY;
    float max_x = -INFINITY, max_y = -INFINITY, max_z = -INFINITY;
    for (Vec3 point : vertices) {
        min_x = fmin(point.x, min_x);
        min_y = fmin(point.y, min_y);
        min_z = fmin(point.z, min_z);
        max_x = fmax(point.x, max_x);
        max_y = fmax(point.y, max_y);
        max_z = fmax(point.z, max_z);
    }
    BoundingBox bbox(Vec3(min_x, min_y, min_z), Vec3(max_x, max_y, max_z));
    root = OctreeNode(bbox);
}

void Mesh::read_file(char *obj_file) {
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
        case '#': {
            parse_comment(&seek);
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
}

bool Mesh::reduce_octree(OctreeNode *node) {
    if (node->total_faces < OCTREE_MINIMUM_FACES) {
        node->contract();
        return true;
    }
    if (node->is_leaf()) {
        return false;
    }
    bool reduce_ok = true;
    for (unsigned char i = 0; i < 8; i++) {
        reduce_ok = reduce_ok && reduce_octree(&node->children[i]);
    }
    if (reduce_ok) {
        node->contract();
#ifdef DEBUG_OCTREE
        printf("Reduced an octree node!\n");
#endif
    }
    return false;
}

Mesh::Mesh(char *obj_file, float ior, float matte, float shiny, float scattering) {
    read_file(obj_file);
    this->ior = ior;
    this->matte = matte;
    this->shiny = shiny;
    this->scattering = scattering;

    // Monocolor
    colors.push_back(Vec3(1, 1, 1));

    // Build normals
    printf("Parsed %zu vertices and %zu faces from %s!\n", vertices.size(), faces.size(), obj_file);
    for (Face &face : faces) {
        Vec3 l = vertices[face.v0] - vertices[face.v1]; // v0-v1
        Vec3 r = vertices[face.v2] - vertices[face.v1]; // v2-v1
        normals.push_back(l % r);
        face.normal = normals.size() - 1;
        face.c = 0;
    }

    // Update Octree bbox
    init_octree();

    // Insert faces
    for (int i = 0; i < faces.size(); i++) {
        insert_face(i);
    }

    // Compute statistics
    root.count_faces();

    // Shrink Octree
    reduce_octree(&root);
}

void Mesh::insert_face(int face_i) {
    Face face = faces[face_i];
    auto verts = {vertices[face.v0], vertices[face.v1], vertices[face.v2]};
    for (const Vec3 &vert : verts) {
        // Find host node
        OctreeNode *node = root.find(vert);
        while (node->depth < OCTREE_MAXIMUM_DEPTH) {
            node->explode();
            node = node->find(vert);
        }
        node->incident_faces.insert(face_i);
    }
}

RaycastResult Mesh::raycast(const LightRay &ray) const { return raycast(ray, &root); }

bool LightRay::intersect(const BoundingBox &bbox) const {
    Vec3 t_min = (bbox.llb - origin) / direction;
    Vec3 t_max = (bbox.urf - origin) / direction;

    if (t_min.x > t_max.x) swap(t_min.x, t_max.x);
    if (t_min.y > t_max.y) swap(t_min.y, t_max.y);
    if (t_min.z > t_max.z) swap(t_min.z, t_max.z);

    float bracket_min = fmax(fmax(t_min.x, t_min.y), t_min.z);
    float bracket_max = fmin(fmin(t_max.x, t_max.y), t_max.z);
    /*if (bracket_max + 1 >= bracket_min)
        printf("Intersect fail ray (%f %f %f)+(%f %f %f)t\n\tBBOX (%f %f %f), (%f %f %f)\n\tMAX %f MIN %f\n", origin.x, origin.y, origin.z,
       direction.x, direction.y, direction.z, bbox.llb.x, bbox.llb.y, bbox.llb.z, bbox.urf.x, bbox.urf.y, bbox.urf.z, bracket_max, bracket_min);
*/
    return bracket_max + EPS >= bracket_min;
}

RaycastResult Mesh::raycast(const LightRay &ray, const OctreeNode *node) const {
    RaycastResult res;
    if (!ray.intersect(node->extent)) {
        return res;
    }
    if (!node->is_leaf()) {
        for (const OctreeNode &subnode : node->children) {
            RaycastResult subres = raycast(ray, &subnode);
            if ((!res.hit && subres.hit) || (subres.hit && res.hit && subres.dist < res.dist)) {
                res = subres;
            }
        }
        return res;
    }
    for (int face_i : node->incident_faces) {
        const Face &face = faces[face_i];
        float dist = intersect(ray.origin, ray.direction, face);
        if ((!res.hit && dist > 0) || (res.hit && dist > 0 && dist < res.dist)) {
            res.hit = true;
            res.dist = dist;
            res.hit_location = ray.origin + ray.direction * dist;
            res.ior = ior;
            res.matte = matte;
            res.scattering = scattering;
            res.shiny = shiny;
            res.color = colors[face.c];
        }
    }
    return res;
}

float Mesh::intersect(const Vec3 &origin, const Vec3 &ray, const Face &tri) const {
    // return 1;
    // Extract vectors from tables
    const Vec3 &normal = normals[tri.normal];
    const Vec3 &v0 = vertices[tri.v0];
    const Vec3 &v1 = vertices[tri.v1];
    const Vec3 &v2 = vertices[tri.v2];

    // Determine ray-plane intersection.
    // Construct the point-norm eq Ax + By + Cz + D = 0
    float D = -(v0 ^ normal); // hehe float D
    // Solve Ax + By + Cz + D = 0, where (x,y,z) = tR+O
    // Can factor out the O:
    float Ds = D + (normal ^ origin);
    // Now the eq is to check if Apt + Bqt + Crt = -D
    // We can merge Ap, Bq, Cr = F to get Ft = -D
    float F = normal ^ ray;
    float t = -Ds / F;
    // If F is zero then the plane and ray are parallel
    // If t is negative, then there is no intersection
    if (fabs(F) < EPS || t < EPS) {
        return -1;
    }
    Vec3 intersect = origin + ray * t;
    // Determine barycentric basis.
    Vec3 A = v1 - v0;
    Vec3 B = v2 - v1;
    Vec3 C = v0 - v2;
    Vec3 AP = intersect - v0;
    Vec3 BP = intersect - v1;
    Vec3 CP = intersect - v2;
    // Now check if intersection point is in the interior of the v1 crux angle
    if (((A % AP) ^ normal) >= 0) {
        return -1;
    }
    if (((B % BP) ^ normal) >= 0) {
        return -1;
    }
    if (((C % CP) ^ normal) >= 0) {
        return -1;
    }
    return t;
}
