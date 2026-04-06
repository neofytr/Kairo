#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include "physics/aabb.h"
#include "physics/circle.h"

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
// collision layer bitmask — entities only collide if their masks overlap
// layer: which layer this entity belongs to
// mask: which layers this entity can collide with
namespace CollisionLayer {
    constexpr u16 None    = 0;
    constexpr u16 Default = 1 << 0;
    constexpr u16 Player  = 1 << 1;
    constexpr u16 Enemy   = 1 << 2;
    constexpr u16 Bullet  = 1 << 3;
    constexpr u16 Wall    = 1 << 4;
    constexpr u16 Pickup  = 1 << 5;
    constexpr u16 All     = 0xFFFF;
}

struct ColliderComponent {
    enum class Shape { AABB, Circle };
    Shape shape = Shape::AABB;

    Vec2 half_size = { 16.0f, 16.0f };
    Vec2 offset = { 0.0f, 0.0f };
    float radius = 16.0f;  // used when shape == Circle
    bool is_trigger = false;

    // collision filtering
    u16 layer = CollisionLayer::Default;  // what I am
    u16 mask  = CollisionLayer::All;      // what I collide with

    // check if two colliders should interact
    bool can_collide_with(const ColliderComponent& other) const {
        return (layer & other.mask) != 0 && (other.layer & mask) != 0;
    }

    AABB get_aabb(const Vec2& position) const {
        Vec2 center = position + offset;
        return AABB::from_center(center, half_size);
    }

    Circle get_circle(const Vec2& position) const {
        return { position + offset, radius };
    }
};

// test collision between two colliders at their world positions
CollisionManifold test_collision(const ColliderComponent& a, const Vec2& pos_a,
                                 const ColliderComponent& b, const Vec2& pos_b);

} // namespace kairo
