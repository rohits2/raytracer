#include "octree.h"

OctreeNode::OctreeNode(){};

OctreeNode::OctreeNode(const BoundingBox &box) {
    extent = box;
    splitting_plane = (box.llb + box.urf) / 2;
}

OctreeNode::OctreeNode(const OctreeNode *parent, unsigned char octant) {
    this->parent = (OctreeNode *)parent;
    this->depth = parent->depth + 1;

    extent = parent->extent;
#ifdef DEBUG_OCTREE
    //printf("Exploding bbox (%f, %f, %f), (%f, %f, %f) into octant %d\n", extent.llb.x, extent.llb.y, extent.llb.z, extent.urf.x, extent.urf.y,
    //       extent.urf.z, octant);
#endif
    if (octant & 0x1) {
        extent.llb.x = parent->splitting_plane.x;
    } else {
        extent.urf.x = parent->splitting_plane.x;
    }
    if (octant & 0x2) {
        extent.llb.y = parent->splitting_plane.y;
    } else {
        extent.urf.y = parent->splitting_plane.y;
    }
    if (octant & 0x4) {
        extent.llb.z = parent->splitting_plane.z;
    } else {
        extent.urf.z = parent->splitting_plane.z;
    }
    splitting_plane = (extent.llb + extent.urf) / 2;
}

void OctreeNode::explode() {
    if (!is_leaf()) {
        return;
    }
    for (unsigned char i = 0; i < 8; i++) {
        children.push_back(OctreeNode(this, i));
    }
}

void OctreeNode::contract() {
    if (is_leaf()) {
        return;
    }
    for (unsigned char i = 0; i < 8; i++) {
        children[i].contract();
        incident_faces.insert(children[i].incident_faces.begin(), children[i].incident_faces.end());
    }
    children.clear();
}
void OctreeNode::count_faces() {
    if (is_leaf()) {
        total_faces = incident_faces.size();
        return;
    }
    set<int> faces;
    total_faces = 0;
    for (unsigned char i = 0; i < 8; i++) {
        children[i].count_faces();
        total_faces += children[i].incident_faces.size();
    }

}

OctreeNode *OctreeNode::find(const Vec3 &point) {
    if (is_leaf()) {
        return extent.contains(point) ? this : nullptr;
    }
    unsigned char o_code = point.compare(splitting_plane);
    return children[o_code].find(point);
}

bool OctreeNode::is_leaf() const { return this->children.size() == 0; }

float BoundingBox::volume() {
    Vec3 edges = urf - llb;
    return fabs(edges.x * edges.y * edges.z);
}

bool BoundingBox::contains(const Vec3 &point) const {
    bool ll_sat = point.x >= llb.x && point.y >= llb.y && point.z >= llb.z;
    bool ur_sat = point.x <= urf.x && point.y <= urf.y && point.z <= urf.z;
    return ll_sat && ur_sat;
}