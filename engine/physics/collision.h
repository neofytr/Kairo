#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include "physics/aabb.h"

#include <vector>

namespace kairo {

// a collision pair detected during broadphase
struct CollisionPair {
    Entity a;
    Entity b;
};

// result of narrowphase — includes manifold info
struct CollisionResult {
    Entity a;
    Entity b;
    CollisionManifold manifold;
};

// component that gives an entity a collider
struct ColliderComponent {
    Vec2 half_size = { 16.0f, 16.0f }; // half-width, half-height
    Vec2 offset = { 0.0f, 0.0f };      // offset from transform position
    bool is_trigger = false;            // triggers detect overlap but don't resolve

    // build the world-space AABB from a position
    AABB get_aabb(const Vec2& position) const {
        Vec2 center = position + offset;
        return AABB::from_center(center, half_size);
    }
};

} // namespace kairo
