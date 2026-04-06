#include "physics/raycast.h"
#include <cmath>
#include <limits>

namespace kairo {

// slab method for ray vs AABB
std::optional<RayHit> ray_vs_aabb(const Ray2D& ray, const AABB& box, float max_distance) {
    float t_min = 0.0f;
    float t_max = max_distance;

    // track which axis/side produced t_min for normal calculation
    int hit_axis = -1;   // 0 = x, 1 = y
    float hit_sign = 0.0f;

    // x slab
    if (std::abs(ray.direction.x) < 1e-8f) {
        // ray parallel to x slab — must be inside
        if (ray.origin.x < box.min.x || ray.origin.x > box.max.x)
            return std::nullopt;
    } else {
        float inv_d = 1.0f / ray.direction.x;
        float t1 = (box.min.x - ray.origin.x) * inv_d;
        float t2 = (box.max.x - ray.origin.x) * inv_d;

        float sign = -1.0f;
        if (t1 > t2) { std::swap(t1, t2); sign = 1.0f; }

        if (t1 > t_min) { t_min = t1; hit_axis = 0; hit_sign = sign; }
        if (t2 < t_max) { t_max = t2; }

        if (t_min > t_max) return std::nullopt;
    }

    // y slab
    if (std::abs(ray.direction.y) < 1e-8f) {
        if (ray.origin.y < box.min.y || ray.origin.y > box.max.y)
            return std::nullopt;
    } else {
        float inv_d = 1.0f / ray.direction.y;
        float t1 = (box.min.y - ray.origin.y) * inv_d;
        float t2 = (box.max.y - ray.origin.y) * inv_d;

        float sign = -1.0f;
        if (t1 > t2) { std::swap(t1, t2); sign = 1.0f; }

        if (t1 > t_min) { t_min = t1; hit_axis = 1; hit_sign = sign; }
        if (t2 < t_max) { t_max = t2; }

        if (t_min > t_max) return std::nullopt;
    }

    // ray starts inside the box if t_min <= 0
    if (t_min < 0.0f) return std::nullopt;

    // compute normal from hit slab face
    Vec2 normal = {0.0f, 0.0f};
    if (hit_axis == 0) normal.x = hit_sign;
    else if (hit_axis == 1) normal.y = hit_sign;

    RayHit hit;
    hit.point = ray.origin + ray.direction * t_min;
    hit.normal = normal;
    hit.distance = t_min;
    hit.entity = NULL_ENTITY;
    return hit;
}

// geometric method for ray vs circle
std::optional<RayHit> ray_vs_circle(const Ray2D& ray, const Vec2& center, float radius, float max_distance) {
    Vec2 oc = ray.origin - center;

    // project center onto ray: a=1 for normalized direction
    float b = oc.dot(ray.direction);
    float c = oc.dot(oc) - radius * radius;

    float discriminant = b * b - c;
    if (discriminant < 0.0f) return std::nullopt;

    float sqrt_d = std::sqrt(discriminant);

    // nearest positive intersection
    float t = -b - sqrt_d;
    if (t < 0.0f) {
        t = -b + sqrt_d;
        if (t < 0.0f) return std::nullopt;
    }

    if (t > max_distance) return std::nullopt;

    Vec2 point = ray.origin + ray.direction * t;
    Vec2 normal = (point - center).normalized();

    RayHit hit;
    hit.point = point;
    hit.normal = normal;
    hit.distance = t;
    hit.entity = NULL_ENTITY;
    return hit;
}

// cast against all targets, return closest hit
std::optional<RayHit> raycast(const Ray2D& ray, const std::vector<RayTarget>& targets, float max_distance) {
    std::optional<RayHit> closest;
    float best_dist = max_distance;

    for (const auto& target : targets) {
        auto hit = ray_vs_aabb(ray, target.aabb, best_dist);
        if (hit && hit->distance < best_dist) {
            hit->entity = target.entity;
            best_dist = hit->distance;
            closest = hit;
        }
    }

    return closest;
}

} // namespace kairo
