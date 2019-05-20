#ifndef RENDER_H
#define RENDER_H
#include "canvas.hpp"
#include "mesh.h"
#include "primitive.h"
#include <queue>
#include <thread>
#include <mutex>

using std::queue, std::thread, std::ref, std::cref, std::mutex;

const int RENDER_TILE_SIZE = 16;

const int AUTO_LINEAR_EXPOSURE = 0;
const int AUTO_GAMMA_EXPOSURE = 1;
const int MANUAL_LINEAR_EXPOSURE = 2;

struct Mesh;
struct LightRay;

struct Light {
    Vec3 loc;
    Vec3 intensity = Vec3(1, 1, 1);
    Light(const Vec3 &loc, const Vec3 &intensity) {
        this->loc = loc;
        this->intensity = intensity;
    };
};

struct Camera {
  public:
    Vec3 loc = Vec3(0, 4, -6);
    Vec3 rotation; // = Vec3(-0.523599, 0, 0); // PYR, default is looking down Z+
    float focal_plane_distance = 1;
    float focal_plane_width = 4;
    float focal_plane_height = 4;
    int exposure_mode = AUTO_LINEAR_EXPOSURE;
    float max_exposure_energy = 55.0f;
    void expose(Canvas &canvas) const;
    LightRay get_initial_ray(const Canvas &canvas, int ray_id) const;
    Camera(){};
    Camera(float focal_distance, float width, float height, float max_exposure)
        : focal_plane_distance(focal_distance), focal_plane_width(width), focal_plane_height(height), max_exposure_energy(max_exposure) {
        exposure_mode = MANUAL_LINEAR_EXPOSURE;
    }
    Camera(float focal_distance, float width, float height)
        : focal_plane_distance(focal_distance), focal_plane_width(width), focal_plane_height(height) {
        exposure_mode = AUTO_LINEAR_EXPOSURE;
    }
    int max_reflections = 3;
};

struct LightRay {
    Vec3 origin;
    Vec3 direction;
    Vec3 intensity = Vec3(1, 1, 1);
    float ior = 1;
    unsigned char bounce_count = 0;
    bool intersect(const BoundingBox &bbox) const;
};

struct Scene {
    Camera camera;
    vector<Mesh> meshes;
    vector<Light> lights;
};

void render(Canvas &canvas, const Scene &scene);
Vec3 raytrace(const LightRay &ray, const Scene &scene);

#endif