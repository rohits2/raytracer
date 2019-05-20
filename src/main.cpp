#include "canvas.hpp"
#include "mesh.h"
#include "render.h"

int main() {
    Scene scene;
    scene.meshes.push_back(Mesh((char *)"obj/cow.obj", 1.5, 0.03, 1, 1));
    scene.meshes.push_back(Mesh((char *)"obj/plane.obj", 4, 0.25, 1, 1));
    for (int i = -9; i < 10; i++) {
        scene.lights.push_back(Light(Vec3(i, 5, i), Vec3(1, 1, 1)));
    }
    scene.lights.push_back(Light(Vec3(1, 0, -3), Vec3(10, 0, 0)));
    scene.lights.push_back(Light(Vec3(-1, 0, -3), Vec3(0, 10, 0)));
    scene.lights.push_back(Light(Vec3(0, 0, -3), Vec3(0, 0, 10)));
    scene.camera = Camera();
    Canvas canvas(80, 80);
    render(canvas, scene);
    canvas.write_ppm((char *)"img.ppm");
}