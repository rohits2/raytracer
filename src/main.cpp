#include "canvas.hpp"
#include "mesh.h"
#include "render.h"

int main() {
    Scene scene;
    scene.meshes.push_back(Mesh((char*)"obj/cow.obj", 1.5, 0, 1, 0));
    scene.meshes.push_back(Mesh((char*)"obj/plane.obj", 4, 1, 1, 1));
    scene.lights.push_back(Light(Vec3(1,0,-3), Vec3(1,1,1)));
    scene.lights.push_back(Light(Vec3(-1,0,-3), Vec3(1,1,1)));
    scene.lights.push_back(Light(Vec3(0,0,-3), Vec3(1,1,1)));
    scene.camera = Camera();
    Canvas canvas(1024, 1024);
    render(canvas, scene);
    canvas.write_ppm((char*)"img.ppm");
}