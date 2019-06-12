#ifndef OCTREE_H
#define OCTREE_H

#include "primitive.h"
#include <algorithm>
#include <fcntl.h>
#include <set>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

using std::vector, std::sort, std::find, std::set;

const int AABB_MINIMUM_FACES = 8;
const int AABB_L1_SIZE = 8;

struct BoundingBox {
    Vec3 llb, urf;
    BoundingBox(const Vec3 &llb, const Vec3 &urf) {
        this->llb = llb;
        this->urf = urf;
    }
    BoundingBox(){};
    float volume();
    bool contains(const Vec3 &point) const;
    BoundingBox join(const BoundingBox &other) const;
};

struct AABBNode {
    BoundingBox extent;
    vector<AABBNode> children;
    unsigned subface_count;
    set<int> incident_faces;
    AABBNode();
    AABBNode(const BoundingBox &extent, int face_i);
    int count_subnodes();
    bool is_leaf() const;
    AABBNode join(const AABBNode &other) const;
    void contract();
    void subsume_children();
    AABBNode *find(const Vec3 &point);
};

#endif