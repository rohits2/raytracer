#include "aabb.h"

AABBNode::AABBNode(){};

AABBNode::AABBNode(const BoundingBox &box, int face_i) {
    extent = box;
    incident_faces.insert(face_i);
}

AABBNode AABBNode::join(const AABBNode &other) const {
    AABBNode parent;
    parent.children.push_back(*this);
    parent.children.push_back(other);
    parent.extent = extent.join(other.extent);
    return parent;
}

void AABBNode::subsume_children() {
    while (children.size() < AABB_L1_SIZE && children.size() > 0) {
        AABBNode &node = children.front();
        if (node.children.size() > 0) {
            children.insert(children.end(), node.children.begin(), node.children.end());
            children.erase(children.begin());
        }else{
            break;
        }
    }
    for (AABBNode &node : children) {
        node.subsume_children();
    }
}

void AABBNode::contract() {
    if (is_leaf()) {
        return;
    }
    for (const AABBNode &node : children) {
        incident_faces.insert(node.incident_faces.begin(), node.incident_faces.end());
    }
    children.clear();
}

AABBNode *AABBNode::find(const Vec3 &point) {
    if (is_leaf()) {
        return extent.contains(point) ? this : nullptr;
    }
    AABBNode *child = nullptr;
    for (AABBNode &node : children) {
        if (node.extent.contains(point)) {
            if (child != nullptr) {
                return this;
            }
            child = &node;
        }
    }
    return child;
}

bool AABBNode::is_leaf() const { return this->children.size() == 0; }

int AABBNode::count_subnodes() {
    if (is_leaf()) {
        subface_count = incident_faces.size();
        return subface_count;
    }
    subface_count = 0;
    for (AABBNode &other : children) {
        subface_count += other.count_subnodes();
    }
    return subface_count;
}

float BoundingBox::volume() {
    Vec3 edges = urf - llb;
    return fabs(edges.x * edges.y * edges.z);
}

bool BoundingBox::contains(const Vec3 &point) const {
    bool ll_sat = point.x >= llb.x && point.y >= llb.y && point.z >= llb.z;
    bool ur_sat = point.x <= urf.x && point.y <= urf.y && point.z <= urf.z;
    return ll_sat && ur_sat;
}

BoundingBox BoundingBox::join(const BoundingBox &other) const {
    BoundingBox parent;
    parent.llb.x = fmin(llb.x, other.llb.x);
    parent.llb.y = fmin(llb.y, other.llb.y);
    parent.llb.z = fmin(llb.z, other.llb.z);
    parent.urf.x = fmax(urf.x, other.urf.x);
    parent.urf.y = fmax(urf.y, other.urf.y);
    parent.urf.z = fmax(urf.z, other.urf.z);
    return parent;
}