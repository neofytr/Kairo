#include "physics/circle.h"

#include <cmath>

namespace kairo {

CollisionManifold circle_vs_circle(const Circle& a, const Circle& b) {
    CollisionManifold result;

    Vec2 diff = b.center - a.center;
    float dist_sq = diff.length_sq();
    float sum_radii = a.radius + b.radius;

    if (dist_sq > sum_radii * sum_radii)
        return result;

    float dist = std::sqrt(dist_sq);

    result.colliding = true;
    result.penetration = sum_radii - dist;

    // handle overlapping centers with arbitrary normal
    if (dist < 1e-6f) {
        result.normal = { 1.0f, 0.0f };
    } else {
        result.normal = diff / dist;
    }

    return result;
}

CollisionManifold circle_vs_aabb(const Circle& circle, const AABB& box) {
    CollisionManifold result;

    // closest point on the AABB to the circle center
    float closest_x = std::fmax(box.min.x, std::fmin(circle.center.x, box.max.x));
    float closest_y = std::fmax(box.min.y, std::fmin(circle.center.y, box.max.y));
    Vec2 closest = { closest_x, closest_y };

    Vec2 diff = circle.center - closest;
    float dist_sq = diff.length_sq();

    if (dist_sq > circle.radius * circle.radius)
        return result;

    float dist = std::sqrt(dist_sq);

    result.colliding = true;
    result.penetration = circle.radius - dist;

    if (dist < 1e-6f) {
        // circle center is inside the AABB — push along shortest axis
        Vec2 box_center = box.center();
        Vec2 to_center = circle.center - box_center;
        Vec2 half = box.half_size();

        float overlap_x = half.x - std::abs(to_center.x);
        float overlap_y = half.y - std::abs(to_center.y);

        if (overlap_x < overlap_y) {
            result.normal = { to_center.x < 0.0f ? -1.0f : 1.0f, 0.0f };
            result.penetration = overlap_x + circle.radius;
        } else {
            result.normal = { 0.0f, to_center.y < 0.0f ? -1.0f : 1.0f };
            result.penetration = overlap_y + circle.radius;
        }
    } else {
        result.normal = diff / dist;
    }

    return result;
}

CollisionManifold aabb_vs_circle(const AABB& box, const Circle& circle) {
    CollisionManifold m = circle_vs_aabb(circle, box);
    m.normal = -m.normal; // flip direction: now pushes box out of circle
    return m;
}

} // namespace kairo
