#pragma once

#include "math/vec2.h"

namespace kairo {

struct AABB {
    Vec2 min; // bottom-left
    Vec2 max; // top-right

    AABB() = default;
    AABB(const Vec2& min, const Vec2& max) : min(min), max(max) {}

    // build from center + half-extents
    static AABB from_center(const Vec2& center, const Vec2& half_size) {
        return { center - half_size, center + half_size };
    }

    Vec2 center() const { return (min + max) * 0.5f; }
    Vec2 size() const { return max - min; }
    Vec2 half_size() const { return size() * 0.5f; }

    bool contains(const Vec2& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y;
    }

    bool intersects(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y;
    }
};

// collision manifold — describes how two AABBs overlap
struct CollisionManifold {
    bool colliding = false;
    Vec2 normal;          // direction to push A out of B
    float penetration = 0.0f;
};

// compute overlap between two AABBs
// returns a manifold with the minimum penetration axis (SAT for AABB)
inline CollisionManifold aabb_vs_aabb(const AABB& a, const AABB& b) {
    CollisionManifold result;

    Vec2 a_center = a.center();
    Vec2 b_center = b.center();
    Vec2 diff = b_center - a_center;

    float overlap_x = a.half_size().x + b.half_size().x - std::abs(diff.x);
    if (overlap_x <= 0.0f) return result;

    float overlap_y = a.half_size().y + b.half_size().y - std::abs(diff.y);
    if (overlap_y <= 0.0f) return result;

    result.colliding = true;

    // resolve along the axis with least penetration
    if (overlap_x < overlap_y) {
        result.penetration = overlap_x;
        result.normal = { diff.x < 0.0f ? -1.0f : 1.0f, 0.0f };
    } else {
        result.penetration = overlap_y;
        result.normal = { 0.0f, diff.y < 0.0f ? -1.0f : 1.0f };
    }

    return result;
}

} // namespace kairo
