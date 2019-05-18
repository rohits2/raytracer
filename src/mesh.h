#ifndef MESH_H
#define MESH_H

#include "primitive.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <set>
#include <algorithm>

using std::vector, std::sort, std::find, std::set;

const int RTREE_MAXIMUM_FACES = 64;
const int RTREE_MAXIMUM_SUBDIVISIONS = 16;

struct BoundingBox {
    Vec3 ll, ur;
    BoundingBox(Vec3 ll, Vec3 ur) {
        this->ll = ll;
        this->ur = ur;
    }
    BoundingBox(){

    }
    float volume();
    bool contains(Vec3 point) const;
};

struct RTreeNode {    
    BoundingBox bbox;
    vector<RTreeNode> children;
    RTreeNode* parent = nullptr;
    set<int> incident_faces;
    void push_face();
};

struct RaycastResult {
    bool hit;
    Vec3 hit_location;
    Vec3 normal;
    float ior;
    float dist;
};

struct LightRay {
    Vec3 origin;
    Vec3 direction;
    Vec3 intensity;
    float ior;
};

struct Mesh {
    Vec3 position, rotation;
    vector<Vec3> vertices, colors, normals;
    vector<Face> faces;
    float ior = 1;
    float diffusion = 0.2;
    float smoothness = 1;

    RTreeNode root;

    Mesh(char *obj_file);
    Mesh(vector<Vec3> vertices, vector<Face> faces, vector<Vec3> colors, float ior, float diffusion, float smoothness);
    RaycastResult raycast(Vec3 origin, Vec3 direction) const;

    void init_rtree();
    void insert_face(int face_i);
    void insert_face(int face_i, RTreeNode* root);
    void split_node(RTreeNode* node);
};

#endif