#ifndef MESH_H
#define MESH_H

#include "aabb.h"
#include "primitive.h"
#include "render.h"
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

using std::vector, std::sort, std::find, std::set, std::swap;

struct LightRay;

struct RaycastResult {
    bool hit = false;
    Vec3 hit_location;
    Vec3 normal;
    Vec3 color;
    float dist;
    float ior = 1;
    float matte = 0.2;
    float shiny =  1;
    float scattering = 0;
};

struct Face {
    Face(int v0, int v1, int v2, int normal, int c) {
        this->v0 = v0;
        this->v1 = v1;
        this->v2 = v2;
        this->normal = normal;
        this->c = c;
    }
    int v0, v1, v2, normal;
    int c;
};

struct Mesh {
    Vec3 position, rotation;
    vector<Vec3> vertices, colors, normals;
    vector<Face> faces;
    float ior = 1;
    float matte = 0.2;
    float shiny = 1;
    float scattering = 0;

    AABBNode root;

    Mesh(char *obj_file, float ior, float matte, float shiny, float scattering);
    Mesh(vector<Vec3> vertices, vector<Face> faces, vector<Vec3> colors, float ior, float diffusion, float smoothness);
    RaycastResult raycast(const LightRay &ray) const;
    RaycastResult raycast(const LightRay &ray, const AABBNode *node) const;
    float intersect(const Vec3 &origin, const Vec3 &direction, const Face &face) const;

    void init_aabb();
    void reduce_aabb(AABBNode* node);
    void read_file(char *obj_file);
    void insert_face(int face_i);
};

#endif