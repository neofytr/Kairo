#pragma once

#include "core/types.h"
#include "math/vec2.h"
#include "physics/aabb.h"
#include "ecs/entity.h"
#include <vector>
#include <optional>

namespace kairo {

struct Ray2D {
    Vec2 origin;
    Vec2 direction;  // should be normalized
};

struct RayHit {
    Vec2 point;      // world-space hit point
    Vec2 normal;     // surface normal at hit
    float distance;  // distance from ray origin
    Entity entity;   // entity that was hit (NULL_ENTITY if geometry-only)
};

// ray vs AABB — returns hit info if intersection occurs within max_distance
std::optional<RayHit> ray_vs_aabb(const Ray2D& ray, const AABB& box, float max_distance = 10000.0f);

// ray vs circle
std::optional<RayHit> ray_vs_circle(const Ray2D& ray, const Vec2& center, float radius, float max_distance = 10000.0f);

// cast a ray against a list of AABBs with associated entities
// returns the closest hit, or nullopt if nothing hit
struct RayTarget {
    AABB aabb;
    Entity entity;
};
std::optional<RayHit> raycast(const Ray2D& ray, const std::vector<RayTarget>& targets, float max_distance = 10000.0f);

} // namespace kairo
