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

const int OCTREE_MINIMUM_FACES = 128;
const int OCTREE_MAXIMUM_DEPTH = 10;

struct BoundingBox {
    Vec3 llb, urf;
    BoundingBox(const Vec3& llb, const Vec3& urf) {
        this->llb = llb;
        this->urf = urf;
    }
    BoundingBox(){};
    float volume();
    bool contains(const Vec3& point) const;
};

struct OctreeNode {
    BoundingBox extent;
    Vec3 splitting_plane;
    unsigned char depth = 0;
    unsigned int total_faces = 0;
    vector<OctreeNode> children;
    set<int> incident_faces;
    OctreeNode *parent = nullptr;
    OctreeNode();
    OctreeNode(const BoundingBox &extent);
    OctreeNode(const OctreeNode *parent, unsigned char octant);
    bool is_leaf() const;
    void explode();
    void contract();
    void count_faces();
    OctreeNode *find(const Vec3 &point);
};

#endif