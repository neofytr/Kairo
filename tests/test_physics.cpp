#include "test_framework.h"
#include "physics/aabb.h"
#include "physics/collision.h"
#include "physics/rigidbody.h"
#include "physics/spatial_grid.h"
#include "ecs/entity.h"

using namespace kairo;

// ============================================================
// AABB tests
// ============================================================

TEST(aabb_from_center) {
    AABB box = AABB::from_center(Vec2(5, 5), Vec2(2, 3));
    ASSERT_NEAR(box.min.x, 3.0f, 1e-6f);
    ASSERT_NEAR(box.min.y, 2.0f, 1e-6f);
    ASSERT_NEAR(box.max.x, 7.0f, 1e-6f);
    ASSERT_NEAR(box.max.y, 8.0f, 1e-6f);
}

TEST(aabb_center) {
    AABB box(Vec2(2, 4), Vec2(8, 10));
    Vec2 c = box.center();
    ASSERT_NEAR(c.x, 5.0f, 1e-6f);
    ASSERT_NEAR(c.y, 7.0f, 1e-6f);
}

TEST(aabb_size) {
    AABB box(Vec2(1, 2), Vec2(5, 8));
    Vec2 s = box.size();
    ASSERT_NEAR(s.x, 4.0f, 1e-6f);
    ASSERT_NEAR(s.y, 6.0f, 1e-6f);
}

TEST(aabb_half_size) {
    AABB box(Vec2(0, 0), Vec2(10, 20));
    Vec2 hs = box.half_size();
    ASSERT_NEAR(hs.x, 5.0f, 1e-6f);
    ASSERT_NEAR(hs.y, 10.0f, 1e-6f);
}

TEST(aabb_contains_inside) {
    AABB box(Vec2(0, 0), Vec2(10, 10));
    ASSERT_TRUE(box.contains(Vec2(5, 5)));
    ASSERT_TRUE(box.contains(Vec2(1, 1)));
}

TEST(aabb_contains_outside) {
    AABB box(Vec2(0, 0), Vec2(10, 10));
    ASSERT_FALSE(box.contains(Vec2(-1, 5)));
    ASSERT_FALSE(box.contains(Vec2(11, 5)));
    ASSERT_FALSE(box.contains(Vec2(5, -1)));
    ASSERT_FALSE(box.contains(Vec2(5, 11)));
}

TEST(aabb_contains_edge) {
    AABB box(Vec2(0, 0), Vec2(10, 10));
    ASSERT_TRUE(box.contains(Vec2(0, 0)));
    ASSERT_TRUE(box.contains(Vec2(10, 10)));
    ASSERT_TRUE(box.contains(Vec2(0, 10)));
    ASSERT_TRUE(box.contains(Vec2(10, 0)));
}

TEST(aabb_intersects_overlapping) {
    AABB a(Vec2(0, 0), Vec2(10, 10));
    AABB b(Vec2(5, 5), Vec2(15, 15));
    ASSERT_TRUE(a.intersects(b));
    ASSERT_TRUE(b.intersects(a));
}

TEST(aabb_intersects_non_overlapping) {
    AABB a(Vec2(0, 0), Vec2(10, 10));
    AABB b(Vec2(20, 20), Vec2(30, 30));
    ASSERT_FALSE(a.intersects(b));
    ASSERT_FALSE(b.intersects(a));
}

TEST(aabb_intersects_touching) {
    AABB a(Vec2(0, 0), Vec2(10, 10));
    AABB b(Vec2(10, 0), Vec2(20, 10));
    // touching edges should count as intersecting (<=)
    ASSERT_TRUE(a.intersects(b));
}

TEST(aabb_intersects_one_inside_other) {
    AABB outer(Vec2(0, 0), Vec2(100, 100));
    AABB inner(Vec2(10, 10), Vec2(20, 20));
    ASSERT_TRUE(outer.intersects(inner));
    ASSERT_TRUE(inner.intersects(outer));
}

TEST(aabb_intersects_separated_x_only) {
    AABB a(Vec2(0, 0), Vec2(5, 10));
    AABB b(Vec2(6, 0), Vec2(10, 10));
    ASSERT_FALSE(a.intersects(b));
}

TEST(aabb_intersects_separated_y_only) {
    AABB a(Vec2(0, 0), Vec2(10, 5));
    AABB b(Vec2(0, 6), Vec2(10, 10));
    ASSERT_FALSE(a.intersects(b));
}

// ============================================================
// CollisionManifold (aabb_vs_aabb) tests
// ============================================================

TEST(collision_overlapping_boxes) {
    AABB a = AABB::from_center(Vec2(0, 0), Vec2(5, 5));
    AABB b = AABB::from_center(Vec2(8, 0), Vec2(5, 5));
    CollisionManifold m = aabb_vs_aabb(a, b);
    ASSERT_TRUE(m.colliding);
    ASSERT_TRUE(m.penetration > 0.0f);
}

TEST(collision_separated_boxes) {
    AABB a = AABB::from_center(Vec2(0, 0), Vec2(5, 5));
    AABB b = AABB::from_center(Vec2(20, 0), Vec2(5, 5));
    CollisionManifold m = aabb_vs_aabb(a, b);
    ASSERT_FALSE(m.colliding);
}

TEST(collision_normal_direction_x) {
    // boxes overlap more on Y, so min penetration axis is X
    AABB a = AABB::from_center(Vec2(0, 0), Vec2(5, 5));
    AABB b = AABB::from_center(Vec2(8, 0), Vec2(5, 5));
    CollisionManifold m = aabb_vs_aabb(a, b);
    ASSERT_TRUE(m.colliding);
    // normal should point along +x (from a toward b)
    ASSERT_NEAR(m.normal.x, 1.0f, 1e-6f);
    ASSERT_NEAR(m.normal.y, 0.0f, 1e-6f);
    ASSERT_NEAR(m.penetration, 2.0f, 1e-5f);
}

TEST(collision_normal_direction_y) {
    // boxes overlap more on X, so min penetration axis is Y
    AABB a = AABB::from_center(Vec2(0, 0), Vec2(5, 5));
    AABB b = AABB::from_center(Vec2(0, 8), Vec2(5, 5));
    CollisionManifold m = aabb_vs_aabb(a, b);
    ASSERT_TRUE(m.colliding);
    ASSERT_NEAR(m.normal.x, 0.0f, 1e-6f);
    ASSERT_NEAR(m.normal.y, 1.0f, 1e-6f);
    ASSERT_NEAR(m.penetration, 2.0f, 1e-5f);
}

TEST(collision_normal_negative_direction) {
    AABB a = AABB::from_center(Vec2(0, 0), Vec2(5, 5));
    AABB b = AABB::from_center(Vec2(-8, 0), Vec2(5, 5));
    CollisionManifold m = aabb_vs_aabb(a, b);
    ASSERT_TRUE(m.colliding);
    ASSERT_NEAR(m.normal.x, -1.0f, 1e-6f);
    ASSERT_NEAR(m.normal.y, 0.0f, 1e-6f);
}

TEST(collision_minimum_penetration_axis) {
    // 3 units overlap on x, 1 unit overlap on y => min pen axis is y
    AABB a = AABB::from_center(Vec2(0, 0), Vec2(5, 5));
    AABB b = AABB::from_center(Vec2(7, 9), Vec2(5, 5));
    CollisionManifold m = aabb_vs_aabb(a, b);
    ASSERT_TRUE(m.colliding);
    // overlap_x = 5+5-7 = 3, overlap_y = 5+5-9 = 1 => y axis
    ASSERT_NEAR(m.normal.x, 0.0f, 1e-6f);
    ASSERT_NEAR(m.normal.y, 1.0f, 1e-6f);
    ASSERT_NEAR(m.penetration, 1.0f, 1e-5f);
}

TEST(collision_barely_touching) {
    // exactly touching: overlap = 0 => not colliding
    AABB a = AABB::from_center(Vec2(0, 0), Vec2(5, 5));
    AABB b = AABB::from_center(Vec2(10, 0), Vec2(5, 5));
    CollisionManifold m = aabb_vs_aabb(a, b);
    ASSERT_FALSE(m.colliding);
}

// ============================================================
// CollisionLayer tests
// ============================================================

TEST(collision_layer_matching) {
    ColliderComponent a;
    a.layer = CollisionLayer::Player;
    a.mask = CollisionLayer::Enemy;

    ColliderComponent b;
    b.layer = CollisionLayer::Enemy;
    b.mask = CollisionLayer::Player;

    ASSERT_TRUE(a.can_collide_with(b));
    ASSERT_TRUE(b.can_collide_with(a));
}

TEST(collision_layer_non_matching) {
    ColliderComponent a;
    a.layer = CollisionLayer::Player;
    a.mask = CollisionLayer::Enemy;

    ColliderComponent b;
    b.layer = CollisionLayer::Wall;
    b.mask = CollisionLayer::Wall;

    ASSERT_FALSE(a.can_collide_with(b));
}

TEST(collision_layer_one_way_mismatch) {
    // a wants to collide with b's layer, but b doesn't want a's layer
    ColliderComponent a;
    a.layer = CollisionLayer::Player;
    a.mask = CollisionLayer::Enemy;

    ColliderComponent b;
    b.layer = CollisionLayer::Enemy;
    b.mask = CollisionLayer::Wall; // doesn't include Player

    ASSERT_FALSE(a.can_collide_with(b));
}

TEST(collision_layer_bullet_vs_enemy) {
    ColliderComponent bullet;
    bullet.layer = CollisionLayer::Bullet;
    bullet.mask = CollisionLayer::Enemy | CollisionLayer::Wall;

    ColliderComponent enemy;
    enemy.layer = CollisionLayer::Enemy;
    enemy.mask = CollisionLayer::Bullet | CollisionLayer::Player;

    ASSERT_TRUE(bullet.can_collide_with(enemy));
}

TEST(collision_layer_bullet_vs_bullet) {
    ColliderComponent bullet1;
    bullet1.layer = CollisionLayer::Bullet;
    bullet1.mask = CollisionLayer::Enemy | CollisionLayer::Wall;

    ColliderComponent bullet2;
    bullet2.layer = CollisionLayer::Bullet;
    bullet2.mask = CollisionLayer::Enemy | CollisionLayer::Wall;

    // bullets don't include Bullet in their masks
    ASSERT_FALSE(bullet1.can_collide_with(bullet2));
}

TEST(collision_layer_all_mask) {
    ColliderComponent a;
    a.layer = CollisionLayer::Pickup;
    a.mask = CollisionLayer::All;

    ColliderComponent b;
    b.layer = CollisionLayer::Player;
    b.mask = CollisionLayer::All;

    ASSERT_TRUE(a.can_collide_with(b));
}

TEST(collision_layer_none) {
    ColliderComponent a;
    a.layer = CollisionLayer::None;
    a.mask = CollisionLayer::All;

    ColliderComponent b;
    b.layer = CollisionLayer::Player;
    b.mask = CollisionLayer::All;

    // a.layer=0, so (a.layer & b.mask) == 0 => false
    ASSERT_FALSE(a.can_collide_with(b));
}

// ============================================================
// SpatialGrid tests
// ============================================================

TEST(spatial_grid_same_cell_pair) {
    SpatialGrid grid(100.0f);
    Entity e1 = Entity::make(1, 1);
    Entity e2 = Entity::make(2, 1);
    // both in same cell
    grid.insert(e1, AABB(Vec2(10, 10), Vec2(20, 20)));
    grid.insert(e2, AABB(Vec2(30, 30), Vec2(40, 40)));
    auto pairs = grid.get_potential_pairs();
    ASSERT_EQ(pairs.size(), (size_t)1);
}

TEST(spatial_grid_different_cells_no_pair) {
    SpatialGrid grid(100.0f);
    Entity e1 = Entity::make(1, 1);
    Entity e2 = Entity::make(2, 1);
    // place in clearly different cells
    grid.insert(e1, AABB(Vec2(10, 10), Vec2(20, 20)));
    grid.insert(e2, AABB(Vec2(200, 200), Vec2(210, 210)));
    auto pairs = grid.get_potential_pairs();
    ASSERT_EQ(pairs.size(), (size_t)0);
}

TEST(spatial_grid_spanning_multiple_cells) {
    SpatialGrid grid(50.0f);
    Entity e1 = Entity::make(1, 1);
    Entity e2 = Entity::make(2, 1);
    // e1 spans cell boundary (from cell 0 to cell 1)
    grid.insert(e1, AABB(Vec2(40, 10), Vec2(60, 20)));
    // e2 in cell 1
    grid.insert(e2, AABB(Vec2(55, 10), Vec2(65, 20)));
    auto pairs = grid.get_potential_pairs();
    // they share at least one cell, so should pair
    ASSERT_TRUE(pairs.size() >= 1);
}

TEST(spatial_grid_clear) {
    SpatialGrid grid(100.0f);
    Entity e1 = Entity::make(1, 1);
    Entity e2 = Entity::make(2, 1);
    grid.insert(e1, AABB(Vec2(10, 10), Vec2(20, 20)));
    grid.insert(e2, AABB(Vec2(30, 30), Vec2(40, 40)));
    grid.clear();
    auto pairs = grid.get_potential_pairs();
    ASSERT_EQ(pairs.size(), (size_t)0);
}

TEST(spatial_grid_three_entities_same_cell) {
    SpatialGrid grid(100.0f);
    Entity e1 = Entity::make(1, 1);
    Entity e2 = Entity::make(2, 1);
    Entity e3 = Entity::make(3, 1);
    grid.insert(e1, AABB(Vec2(10, 10), Vec2(20, 20)));
    grid.insert(e2, AABB(Vec2(30, 30), Vec2(40, 40)));
    grid.insert(e3, AABB(Vec2(50, 50), Vec2(60, 60)));
    auto pairs = grid.get_potential_pairs();
    // 3 entities => C(3,2) = 3 pairs
    ASSERT_EQ(pairs.size(), (size_t)3);
}

TEST(spatial_grid_single_entity_no_pair) {
    SpatialGrid grid(100.0f);
    Entity e1 = Entity::make(1, 1);
    grid.insert(e1, AABB(Vec2(10, 10), Vec2(20, 20)));
    auto pairs = grid.get_potential_pairs();
    ASSERT_EQ(pairs.size(), (size_t)0);
}

// ============================================================
// RigidBody tests
// ============================================================

TEST(rigidbody_default) {
    RigidBodyComponent rb;
    ASSERT_NEAR(rb.mass, 1.0f, 1e-6f);
    ASSERT_NEAR(rb.inv_mass, 1.0f, 1e-6f);
    ASSERT_FALSE(rb.is_static);
}

TEST(rigidbody_set_mass) {
    RigidBodyComponent rb;
    rb.set_mass(4.0f);
    ASSERT_NEAR(rb.mass, 4.0f, 1e-6f);
    ASSERT_NEAR(rb.inv_mass, 0.25f, 1e-6f);
}

TEST(rigidbody_set_mass_zero) {
    RigidBodyComponent rb;
    rb.set_mass(0.0f);
    ASSERT_NEAR(rb.mass, 0.0f, 1e-6f);
    ASSERT_NEAR(rb.inv_mass, 0.0f, 1e-6f);
}

TEST(rigidbody_set_mass_large) {
    RigidBodyComponent rb;
    rb.set_mass(1000.0f);
    ASSERT_NEAR(rb.inv_mass, 0.001f, 1e-6f);
}

TEST(rigidbody_make_static) {
    RigidBodyComponent rb;
    rb.velocity = Vec2(10, 20);
    rb.make_static();
    ASSERT_TRUE(rb.is_static);
    ASSERT_NEAR(rb.mass, 0.0f, 1e-6f);
    ASSERT_NEAR(rb.inv_mass, 0.0f, 1e-6f);
    ASSERT_NEAR(rb.velocity.x, 0.0f, 1e-6f);
    ASSERT_NEAR(rb.velocity.y, 0.0f, 1e-6f);
}

TEST(rigidbody_apply_force) {
    RigidBodyComponent rb;
    rb.apply_force(Vec2(10, 0));
    ASSERT_NEAR(rb.force.x, 10.0f, 1e-6f);
    ASSERT_NEAR(rb.force.y, 0.0f, 1e-6f);
}

TEST(rigidbody_apply_force_accumulates) {
    RigidBodyComponent rb;
    rb.apply_force(Vec2(10, 5));
    rb.apply_force(Vec2(3, -2));
    rb.apply_force(Vec2(-1, 7));
    ASSERT_NEAR(rb.force.x, 12.0f, 1e-6f);
    ASSERT_NEAR(rb.force.y, 10.0f, 1e-6f);
}

TEST(rigidbody_initial_velocity_zero) {
    RigidBodyComponent rb;
    ASSERT_NEAR(rb.velocity.x, 0.0f, 1e-6f);
    ASSERT_NEAR(rb.velocity.y, 0.0f, 1e-6f);
}

TEST(rigidbody_restitution_default) {
    RigidBodyComponent rb;
    ASSERT_NEAR(rb.restitution, 0.3f, 1e-6f);
}

TEST(rigidbody_friction_default) {
    RigidBodyComponent rb;
    ASSERT_NEAR(rb.friction, 0.2f, 1e-6f);
}

TEST(collider_get_aabb) {
    ColliderComponent col;
    col.half_size = Vec2(16, 16);
    col.offset = Vec2(0, 0);
    AABB box = col.get_aabb(Vec2(100, 200));
    ASSERT_NEAR(box.min.x, 84.0f, 1e-6f);
    ASSERT_NEAR(box.min.y, 184.0f, 1e-6f);
    ASSERT_NEAR(box.max.x, 116.0f, 1e-6f);
    ASSERT_NEAR(box.max.y, 216.0f, 1e-6f);
}

TEST(collider_get_aabb_with_offset) {
    ColliderComponent col;
    col.half_size = Vec2(10, 10);
    col.offset = Vec2(5, -5);
    AABB box = col.get_aabb(Vec2(50, 50));
    Vec2 c = box.center();
    ASSERT_NEAR(c.x, 55.0f, 1e-6f);
    ASSERT_NEAR(c.y, 45.0f, 1e-6f);
}
