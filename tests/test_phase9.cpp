#include "test_framework.h"
#include "physics/aabb.h"
#include "physics/raycast.h"
#include "physics/circle.h"
#include "physics/joints.h"
#include "physics/rigidbody.h"
#include "graphics/screen_utils.h"
#include "ecs/entity.h"

using namespace kairo;

// ============================================================
// Ray vs AABB tests
// ============================================================

TEST(ray_vs_aabb_hit_from_left) {
    // ray shooting right, hitting box on its left face
    Ray2D ray;
    ray.origin = Vec2(-10, 5);
    ray.direction = Vec2(1, 0);

    AABB box(Vec2(0, 0), Vec2(10, 10));
    auto hit = ray_vs_aabb(ray, box);

    ASSERT_TRUE(hit.has_value());
    ASSERT_NEAR(hit->point.x, 0.0f, 1e-4f);
    ASSERT_NEAR(hit->point.y, 5.0f, 1e-4f);
    ASSERT_NEAR(hit->normal.x, -1.0f, 1e-6f);
    ASSERT_NEAR(hit->normal.y, 0.0f, 1e-6f);
    ASSERT_NEAR(hit->distance, 10.0f, 1e-4f);
}

TEST(ray_vs_aabb_miss) {
    // ray shooting up, missing the box entirely
    Ray2D ray;
    ray.origin = Vec2(-10, 5);
    ray.direction = Vec2(0, 1);

    AABB box(Vec2(0, 0), Vec2(10, 10));
    auto hit = ray_vs_aabb(ray, box);

    ASSERT_FALSE(hit.has_value());
}

TEST(ray_vs_aabb_origin_inside) {
    // ray starts inside the box -- should still return a hit (exit point)
    Ray2D ray;
    ray.origin = Vec2(5, 5);
    ray.direction = Vec2(1, 0);

    AABB box(Vec2(0, 0), Vec2(10, 10));
    auto hit = ray_vs_aabb(ray, box);

    ASSERT_TRUE(hit.has_value());
    // slab method returns t=0 when origin is inside, so distance >= 0
    ASSERT_TRUE(hit->distance >= 0.0f);
}

TEST(ray_vs_aabb_hit_from_above) {
    // ray shooting down, hitting box on its top face
    Ray2D ray;
    ray.origin = Vec2(5, 20);
    ray.direction = Vec2(0, -1);

    AABB box(Vec2(0, 0), Vec2(10, 10));
    auto hit = ray_vs_aabb(ray, box);

    ASSERT_TRUE(hit.has_value());
    ASSERT_NEAR(hit->point.x, 5.0f, 1e-4f);
    ASSERT_NEAR(hit->point.y, 10.0f, 1e-4f);
    ASSERT_NEAR(hit->normal.x, 0.0f, 1e-6f);
    ASSERT_NEAR(hit->normal.y, 1.0f, 1e-6f);
    ASSERT_NEAR(hit->distance, 10.0f, 1e-4f);
}

TEST(ray_vs_aabb_max_distance_too_short) {
    // ray would hit the box, but max_distance is too short
    Ray2D ray;
    ray.origin = Vec2(-10, 5);
    ray.direction = Vec2(1, 0);

    AABB box(Vec2(0, 0), Vec2(10, 10));
    auto hit = ray_vs_aabb(ray, box, 5.0f); // need 10 units, only allow 5

    ASSERT_FALSE(hit.has_value());
}

// ============================================================
// Ray vs Circle tests
// ============================================================

TEST(ray_vs_circle_hit) {
    Ray2D ray;
    ray.origin = Vec2(-10, 0);
    ray.direction = Vec2(1, 0);

    Vec2 center(5, 0);
    float radius = 3.0f;

    auto hit = ray_vs_circle(ray, center, radius);

    ASSERT_TRUE(hit.has_value());
    // entry point at x = 5 - 3 = 2
    ASSERT_NEAR(hit->point.x, 2.0f, 1e-4f);
    ASSERT_NEAR(hit->point.y, 0.0f, 1e-4f);
    ASSERT_NEAR(hit->distance, 12.0f, 1e-4f);
}

TEST(ray_vs_circle_miss) {
    Ray2D ray;
    ray.origin = Vec2(-10, 0);
    ray.direction = Vec2(1, 0);

    Vec2 center(5, 10); // circle is far above ray
    float radius = 3.0f;

    auto hit = ray_vs_circle(ray, center, radius);

    ASSERT_FALSE(hit.has_value());
}

TEST(ray_vs_circle_origin_inside) {
    Ray2D ray;
    ray.origin = Vec2(5, 0);
    ray.direction = Vec2(1, 0);

    Vec2 center(5, 0);
    float radius = 3.0f;

    auto hit = ray_vs_circle(ray, center, radius);

    ASSERT_TRUE(hit.has_value());
    // exit point at x = 5 + 3 = 8
    ASSERT_NEAR(hit->point.x, 8.0f, 1e-4f);
    ASSERT_NEAR(hit->point.y, 0.0f, 1e-4f);
}

TEST(ray_vs_circle_tangent_miss) {
    // ray passes just barely outside the circle
    Ray2D ray;
    ray.origin = Vec2(-10, 3.01f); // slightly above the top of a radius-3 circle at origin
    ray.direction = Vec2(1, 0);

    Vec2 center(0, 0);
    float radius = 3.0f;

    auto hit = ray_vs_circle(ray, center, radius);

    ASSERT_FALSE(hit.has_value());
}

// ============================================================
// Circle vs Circle tests
// ============================================================

TEST(circle_vs_circle_overlapping) {
    Circle a = { Vec2(0, 0), 5.0f };
    Circle b = { Vec2(7, 0), 5.0f };

    CollisionManifold m = circle_vs_circle(a, b);

    ASSERT_TRUE(m.colliding);
    // penetration = (5 + 5) - 7 = 3
    ASSERT_NEAR(m.penetration, 3.0f, 1e-4f);
    // normal should point from a to b (+x direction)
    ASSERT_NEAR(m.normal.x, 1.0f, 1e-4f);
    ASSERT_NEAR(m.normal.y, 0.0f, 1e-4f);
}

TEST(circle_vs_circle_separated) {
    Circle a = { Vec2(0, 0), 3.0f };
    Circle b = { Vec2(10, 0), 3.0f };

    CollisionManifold m = circle_vs_circle(a, b);

    ASSERT_FALSE(m.colliding);
}

TEST(circle_vs_circle_identical_position) {
    Circle a = { Vec2(5, 5), 3.0f };
    Circle b = { Vec2(5, 5), 4.0f };

    CollisionManifold m = circle_vs_circle(a, b);

    ASSERT_TRUE(m.colliding);
    // penetration should be sum of radii when centers coincide
    ASSERT_NEAR(m.penetration, 7.0f, 1e-4f);
    // normal is arbitrary but should be unit length (or some default)
    float normal_len = m.normal.length();
    ASSERT_NEAR(normal_len, 1.0f, 1e-4f);
}

TEST(circle_vs_circle_touching) {
    // distance exactly equals sum of radii -- edge case
    Circle a = { Vec2(0, 0), 5.0f };
    Circle b = { Vec2(10, 0), 5.0f };

    CollisionManifold m = circle_vs_circle(a, b);

    // touching means penetration = 0, may or may not be considered colliding
    // depending on implementation (distance <= sum vs <)
    // either way, penetration should be ~0
    if (m.colliding) {
        ASSERT_NEAR(m.penetration, 0.0f, 1e-4f);
    } else {
        ASSERT_FALSE(m.colliding);
    }
}

// ============================================================
// Circle vs AABB tests
// ============================================================

TEST(circle_vs_aabb_overlapping) {
    Circle circ = { Vec2(12, 5), 4.0f };
    AABB box(Vec2(0, 0), Vec2(10, 10));

    CollisionManifold m = circle_vs_aabb(circ, box);

    ASSERT_TRUE(m.colliding);
    // closest point on box to circle center is (10, 5), distance = 2, penetration = 4 - 2 = 2
    ASSERT_NEAR(m.penetration, 2.0f, 1e-4f);
}

TEST(circle_vs_aabb_separated) {
    Circle circ = { Vec2(20, 5), 3.0f };
    AABB box(Vec2(0, 0), Vec2(10, 10));

    CollisionManifold m = circle_vs_aabb(circ, box);

    ASSERT_FALSE(m.colliding);
}

TEST(circle_vs_aabb_corner) {
    // circle near the corner of the box
    // box corner at (10, 10), circle center at (13, 14)
    // distance from corner = sqrt(9 + 16) = 5
    Circle circ = { Vec2(13, 14), 6.0f };
    AABB box(Vec2(0, 0), Vec2(10, 10));

    CollisionManifold m = circle_vs_aabb(circ, box);

    ASSERT_TRUE(m.colliding);
    // penetration = 6 - 5 = 1
    ASSERT_NEAR(m.penetration, 1.0f, 1e-4f);
}

// ============================================================
// Raycast (multi-target) tests
// ============================================================

TEST(raycast_multiple_targets_closest) {
    Ray2D ray;
    ray.origin = Vec2(-10, 5);
    ray.direction = Vec2(1, 0);

    Entity e1 = Entity::make(1, 1);
    Entity e2 = Entity::make(2, 1);
    Entity e3 = Entity::make(3, 1);

    std::vector<RayTarget> targets = {
        { AABB(Vec2(20, 0), Vec2(30, 10)), e1 },  // distance ~30 to enter
        { AABB(Vec2(0, 0), Vec2(10, 10)), e2 },    // distance ~10 to enter (closest)
        { AABB(Vec2(40, 0), Vec2(50, 10)), e3 },   // distance ~50 to enter
    };

    auto hit = raycast(ray, targets);

    ASSERT_TRUE(hit.has_value());
    ASSERT_TRUE(hit->entity == e2);
    ASSERT_NEAR(hit->point.x, 0.0f, 1e-4f);
}

TEST(raycast_no_targets_hit) {
    Ray2D ray;
    ray.origin = Vec2(-10, 50); // ray is above all targets
    ray.direction = Vec2(1, 0);

    Entity e1 = Entity::make(1, 1);
    Entity e2 = Entity::make(2, 1);

    std::vector<RayTarget> targets = {
        { AABB(Vec2(0, 0), Vec2(10, 10)), e1 },
        { AABB(Vec2(20, 0), Vec2(30, 10)), e2 },
    };

    auto hit = raycast(ray, targets);

    ASSERT_FALSE(hit.has_value());
}

// ============================================================
// Screen to World / World to Screen tests
// ============================================================

TEST(screen_to_world_center) {
    Vec2 camera_pos(100, 200);
    Vec2 ortho_half(400, 300);
    i32 sw = 800;
    i32 sh = 600;

    // center of screen
    Vec2 world = screen_to_world(Vec2(400, 300), camera_pos, ortho_half, sw, sh);

    ASSERT_NEAR(world.x, 100.0f, 1e-4f);
    ASSERT_NEAR(world.y, 200.0f, 1e-4f);
}

TEST(screen_to_world_top_left) {
    Vec2 camera_pos(0, 0);
    Vec2 ortho_half(400, 300);
    i32 sw = 800;
    i32 sh = 600;

    // top-left of screen: (0, 0)
    // nx = (0/800)*2 - 1 = -1
    // ny = 1 - (0/600)*2 = 1
    // world = (0 + (-1)*400, 0 + 1*300) = (-400, 300)
    Vec2 world = screen_to_world(Vec2(0, 0), camera_pos, ortho_half, sw, sh);

    ASSERT_NEAR(world.x, -400.0f, 1e-4f);
    ASSERT_NEAR(world.y, 300.0f, 1e-4f);
}

TEST(screen_world_roundtrip) {
    Vec2 camera_pos(50, -30);
    Vec2 ortho_half(320, 240);
    i32 sw = 640;
    i32 sh = 480;

    Vec2 original_screen(123, 456);
    Vec2 world = screen_to_world(original_screen, camera_pos, ortho_half, sw, sh);
    Vec2 back = world_to_screen(world, camera_pos, ortho_half, sw, sh);

    ASSERT_NEAR(back.x, original_screen.x, 1e-2f);
    ASSERT_NEAR(back.y, original_screen.y, 1e-2f);
}

// ============================================================
// Joint System tests
// ============================================================

TEST(joint_system_add_distance_joint) {
    JointSystem js;
    Entity a = Entity::make(1, 1);
    Entity b = Entity::make(2, 1);

    ASSERT_EQ(js.joint_count(), (size_t)0);
    js.add_distance_joint(a, b, 100.0f);
    ASSERT_EQ(js.joint_count(), (size_t)1);

    const auto& joints = js.get_joints();
    ASSERT_EQ(joints[0].type, JointType::Distance);
    ASSERT_NEAR(joints[0].rest_length, 100.0f, 1e-6f);
    ASSERT_TRUE(joints[0].entity_a == a);
    ASSERT_TRUE(joints[0].entity_b == b);
}

TEST(joint_system_add_spring_joint) {
    JointSystem js;
    Entity a = Entity::make(1, 1);
    Entity b = Entity::make(2, 1);

    js.add_spring_joint(a, b, 50.0f, 80.0f, 10.0f);
    ASSERT_EQ(js.joint_count(), (size_t)1);

    const auto& joints = js.get_joints();
    ASSERT_EQ(joints[0].type, JointType::Spring);
    ASSERT_NEAR(joints[0].rest_length, 50.0f, 1e-6f);
    ASSERT_NEAR(joints[0].stiffness, 80.0f, 1e-6f);
    ASSERT_NEAR(joints[0].damping, 10.0f, 1e-6f);
}

TEST(joint_system_remove_joints) {
    JointSystem js;
    Entity a = Entity::make(1, 1);
    Entity b = Entity::make(2, 1);
    Entity c = Entity::make(3, 1);

    js.add_distance_joint(a, b, 100.0f);
    js.add_spring_joint(b, c, 50.0f);
    js.add_distance_joint(a, c, 75.0f);
    ASSERT_EQ(js.joint_count(), (size_t)3);

    // remove all joints involving entity a
    js.remove_joints(a);
    ASSERT_EQ(js.joint_count(), (size_t)1);

    // remaining joint should be b-c
    const auto& joints = js.get_joints();
    ASSERT_TRUE(joints[0].entity_a == b);
    ASSERT_TRUE(joints[0].entity_b == c);
}

TEST(joint_system_clear) {
    JointSystem js;
    Entity a = Entity::make(1, 1);
    Entity b = Entity::make(2, 1);
    Entity c = Entity::make(3, 1);

    js.add_distance_joint(a, b, 100.0f);
    js.add_spring_joint(b, c, 50.0f);
    ASSERT_EQ(js.joint_count(), (size_t)2);

    js.clear();
    ASSERT_EQ(js.joint_count(), (size_t)0);
}
