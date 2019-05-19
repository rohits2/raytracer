#include "render.h"

void Camera::expose(Canvas &canvas) const {
    Vec3 max_exposure = Vec3(max_exposure_energy, max_exposure_energy, max_exposure_energy);
    switch (exposure_mode) {
    case AUTO_LINEAR_EXPOSURE: {
        max_exposure = -INFINITY;
        for (int i = 0; i < canvas.height; i++) {
            for (int j = 0; j < canvas.height; j++) {
                max_exposure.x = fmax(max_exposure.x, canvas[i][j].x);
                max_exposure.y = fmax(max_exposure.y, canvas[i][j].y);
                max_exposure.z = fmax(max_exposure.z, canvas[i][j].z);
            }
        }
        printf("Max luminance (%f, %f, %f)\n", max_exposure.x, max_exposure.y, max_exposure.z);
        break;
    }
    case MANUAL_LINEAR_EXPOSURE:
        break;
    }
    for (int i = 0; i < canvas.height; i++) {
        for (int j = 0; j < canvas.height; j++) {
            canvas[i][j] = canvas[i][j] / max_exposure;
            if (canvas[i][j].x > 1) canvas[i][j].x = 1;
            if (canvas[i][j].y > 1) canvas[i][j].y = 1;
            if (canvas[i][j].z > 1) canvas[i][j].z = 1;
        }
    }
}

LightRay Camera::get_initial_ray(const Canvas &canvas, int ray_id) const {
    int i = ray_id / canvas.width;
    int j = ray_id % canvas.width;
    LightRay ray;
    ray.origin = loc;
    int fold_i = canvas.height / 2.0;
    int fold_j = canvas.width / 2.0;
    float scaled_x = (j - fold_j) * focal_plane_width / canvas.width;
    float scaled_y = (fold_i - i) * focal_plane_height / canvas.height;
    ray.direction = Vec3(scaled_x, scaled_y, focal_plane_distance).normalize();
    ray.direction = ray.direction.rotate(rotation);
    return ray;
}

LightRay get_reflection(const LightRay &parent, const RaycastResult &hit) {
    LightRay reflection;
    reflection.bounce_count = parent.bounce_count + 1;
    reflection.intensity = parent.intensity;
    reflection.origin = hit.hit_location;
    reflection.ior = parent.ior;
    reflection.direction = parent.direction - 2 * (parent.direction ^ hit.normal) * hit.normal;
    return reflection;
}

LightRay get_refraction(const LightRay &parent, const RaycastResult &hit) {
    LightRay refraction;
    refraction.bounce_count = parent.bounce_count + 1;
    refraction.intensity = parent.intensity;
    refraction.origin = hit.hit_location;
    refraction.ior = hit.ior;
    float c = -hit.normal.sum();
    float r = parent.ior / hit.ior;
    Vec3 ray = (r * parent.direction) + (r * c - sqrt(1 - r * r * (1 - c * c))) * hit.normal;
    if ((ray ^ parent.direction) <= 0) {
        ray = -ray;
    }
    refraction.direction = ray;
    return refraction;
}

float fresnel(const LightRay &incident, const RaycastResult &hit) {
    float cosi = incident.direction ^ hit.normal;
    float etai = 1;
    float etat = hit.ior;
    if (cosi > 0) {
        swap(etai, etat);
    }
    float sint = etai / etat * sqrt(fmax(0.f, 1 - cosi * cosi));
    // Total internal reflection
    if (sint >= 1) {
        return 1;
    } else {
        float cost = sqrt(fmax(0.f, 1 - sint * sint));
        cosi = fabsf(cosi);
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
        return (Rs * Rs + Rp * Rp) / 2;
    }
}

Vec3 local_illuminate(const RaycastResult &hit, const Scene &scene) {
    // distance falloff only
    Vec3 total_illumination;
    LightRay shadow_ray;
    shadow_ray.origin = hit.hit_location;
    for (const Light &light : scene.lights) {

        Vec3 ray = (light.loc - hit.hit_location);
        float dist = ray.magnitude();
        ray = ray.normalize();
        shadow_ray.direction = ray;
        for (const Mesh &mesh : scene.meshes) {
            RaycastResult rr = mesh.raycast(shadow_ray);
            if (!rr.hit) {
                Vec3 intensity = light.intensity * (shadow_ray.direction ^ hit.normal) / (4 * PI * dist * dist);
                total_illumination = total_illumination + intensity;
            }
        }
    }
    return total_illumination * hit.color;
}

Vec3 raytrace(const LightRay &ray, const Scene &scene) {
    if (ray.intensity.sum() < EPS) {
        return Vec3(0, 0, 0);
    }

    RaycastResult rr;
    for (const Mesh &mesh : scene.meshes) {
        RaycastResult sub_rr = mesh.raycast(ray);
        if ((!rr.hit && sub_rr.hit) || (rr.hit && sub_rr.hit && sub_rr.dist < rr.dist)) {
            rr = sub_rr;
        }
    }
    if (!rr.hit) {
        return Vec3(0, 0, 0);
    }
    float diffuse_intensity = rr.matte;
    float reflection_intensity = fresnel(ray, rr);
    float refraction_intensity = 1 - reflection_intensity;

    Vec3 color = local_illuminate(rr, scene) * ray.intensity * diffuse_intensity;

    if (reflection_intensity >= EPS) {
        LightRay reflection = get_reflection(ray, rr);
        reflection.intensity = reflection.intensity * ray.intensity * reflection_intensity * 0.9;
        color = color + raytrace(reflection, scene);
    }
    if (refraction_intensity >= EPS) {
        LightRay refraction = get_refraction(ray, rr);
        refraction.intensity = refraction.intensity * ray.intensity * refraction_intensity * 0.9;
        color = color + raytrace(refraction, scene);
    }
    return color;
}

int min(int a, int b) { return a < b ? a : b; }

void subrender(Canvas &canvas, const Scene &scene, vector<int> tile) {
    for (int id : tile) {
        LightRay ray = scene.camera.get_initial_ray(canvas, id);
        int i = id / canvas.width;
        int j = id % canvas.width;
        canvas[i][j] = raytrace(ray, scene);
    }
}

void queue_render(Canvas &canvas, const Scene &scene, queue<vector<int>> &tile_queue, mutex &tile_queue_lock) {
    tile_queue_lock.lock();
    while (!tile_queue.empty()) {
        printf("Tile queue has %lu remaining...\n", tile_queue.size());
        vector<int> tile = tile_queue.front();
        tile_queue.pop();
        tile_queue_lock.unlock();
        subrender(canvas, scene, tile);
        tile_queue_lock.lock();
    }
    tile_queue_lock.unlock();
}

void render(Canvas &canvas, const Scene &scene) {
    queue<vector<int>> tile_queue;
    for (int i = 0; i < canvas.height; i += RENDER_TILE_SIZE) {
        for (int j = 0; j < canvas.width; j += RENDER_TILE_SIZE) {
            vector<int> tile;
            for (int ii = i; ii < min(canvas.height, i + RENDER_TILE_SIZE); ii++) {
                for (int jj = j; jj < min(canvas.width, j + RENDER_TILE_SIZE); jj++) {
                    int ind = ii * canvas.width + jj;
                    tile.push_back(ind);
                }
            }
            tile_queue.push(tile);
        }
    }
    mutex queue_lock;
    thread workers[15];
    for (int i = 0; i < 15; i++) {
        workers[i] = thread(queue_render, ref(canvas), cref(scene), ref(tile_queue), ref(queue_lock));
    }
    for (int i = 0; i < 15; i++) {
        workers[i].join();
    }
    scene.camera.expose(canvas);
}