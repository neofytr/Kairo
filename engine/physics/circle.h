#pragma once

#include "math/vec2.h"
#include "physics/aabb.h"

namespace kairo {

struct Circle {
    Vec2 center;
    float radius;
};

// circle vs circle collision
CollisionManifold circle_vs_circle(const Circle& a, const Circle& b);

// circle vs AABB collision
CollisionManifold circle_vs_aabb(const Circle& circle, const AABB& box);

// AABB vs circle (flips normal)
CollisionManifold aabb_vs_circle(const AABB& box, const Circle& circle);

} // namespace kairo
