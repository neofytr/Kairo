#include "test_framework.h"
#include "memory/pool_allocator.h"
#include "memory/linear_allocator.h"
#include "memory/stack_allocator.h"

#include <cstdint>

// helper struct for pool allocator tests
struct TestObj {
    int x;
    float y;
    TestObj() : x(0), y(0.0f) {}
    TestObj(int x_, float y_) : x(x_), y(y_) {}
};

// ============================================================
// PoolAllocator tests
// ============================================================

TEST(pool_allocate_returns_non_null) {
    kairo::PoolAllocator<TestObj> pool(16);
    TestObj* p = pool.allocate(1, 2.0f);
    ASSERT_TRUE(p != nullptr);
    pool.deallocate(p);
}

TEST(pool_allocated_object_has_correct_values) {
    kairo::PoolAllocator<TestObj> pool(16);
    TestObj* p = pool.allocate(42, 3.14f);
    ASSERT_EQ(p->x, 42);
    ASSERT_NEAR(p->y, 3.14f, 0.001f);
    pool.deallocate(p);
}

TEST(pool_deallocate_decreases_alive_count) {
    kairo::PoolAllocator<TestObj> pool(16);
    TestObj* p = pool.allocate(1, 1.0f);
    ASSERT_EQ(pool.alive_count(), 1u);
    pool.deallocate(p);
    ASSERT_EQ(pool.alive_count(), 0u);
}

TEST(pool_reallocate_reuses_slot) {
    kairo::PoolAllocator<TestObj> pool(16);
    TestObj* first = pool.allocate(1, 1.0f);
    pool.deallocate(first);
    TestObj* second = pool.allocate(2, 2.0f);
    // the freed slot should be reused
    ASSERT_EQ(first, second);
    ASSERT_EQ(second->x, 2);
    pool.deallocate(second);
}

TEST(pool_grows_when_exhausted) {
    kairo::PoolAllocator<TestObj> pool(2);
    TestObj* a = pool.allocate(1, 1.0f);
    TestObj* b = pool.allocate(2, 2.0f);
    // pool is full (capacity 2), next allocate should trigger growth
    TestObj* c = pool.allocate(3, 3.0f);
    ASSERT_TRUE(c != nullptr);
    ASSERT_EQ(c->x, 3);
    ASSERT_EQ(pool.alive_count(), 3u);
    pool.deallocate(a);
    pool.deallocate(b);
    pool.deallocate(c);
}

TEST(pool_capacity_increases_after_growth) {
    kairo::PoolAllocator<TestObj> pool(2);
    ASSERT_EQ(pool.capacity(), 2u);
    pool.allocate(1, 1.0f);
    pool.allocate(2, 2.0f);
    pool.allocate(3, 3.0f); // triggers growth
    ASSERT(pool.capacity() > 2u);
}

TEST(pool_multiple_allocate_deallocate_cycles) {
    kairo::PoolAllocator<TestObj> pool(8);
    for (int cycle = 0; cycle < 5; cycle++) {
        TestObj* ptrs[4];
        for (int i = 0; i < 4; i++) {
            ptrs[i] = pool.allocate(i, static_cast<float>(i));
        }
        ASSERT_EQ(pool.alive_count(), 4u);
        for (int i = 3; i >= 0; i--) {
            pool.deallocate(ptrs[i]);
        }
        ASSERT_EQ(pool.alive_count(), 0u);
    }
}

// ============================================================
// LinearAllocator tests
// ============================================================

TEST(linear_allocate_returns_non_null_aligned) {
    kairo::LinearAllocator alloc(1024);
    void* p = alloc.allocate(64);
    ASSERT_TRUE(p != nullptr);
    // default alignment is 8
    ASSERT_EQ(reinterpret_cast<uintptr_t>(p) % 8, 0u);
}

TEST(linear_used_increases_after_allocation) {
    kairo::LinearAllocator alloc(1024);
    ASSERT_EQ(alloc.used(), 0u);
    alloc.allocate(32);
    ASSERT(alloc.used() >= 32u);
}

TEST(linear_multiple_allocations_are_sequential) {
    kairo::LinearAllocator alloc(1024);
    void* a = alloc.allocate(16);
    void* b = alloc.allocate(16);
    // b should come after a in memory
    ASSERT_TRUE(reinterpret_cast<uintptr_t>(b) > reinterpret_cast<uintptr_t>(a));
}

TEST(linear_reset_sets_used_to_zero) {
    kairo::LinearAllocator alloc(1024);
    alloc.allocate(64);
    alloc.allocate(64);
    ASSERT(alloc.used() > 0u);
    alloc.reset();
    ASSERT_EQ(alloc.used(), 0u);
}

TEST(linear_allocate_after_reset_works) {
    kairo::LinearAllocator alloc(1024);
    alloc.allocate(128);
    alloc.reset();
    void* p = alloc.allocate(64);
    ASSERT_TRUE(p != nullptr);
    ASSERT(alloc.used() >= 64u);
    ASSERT(alloc.used() <= 128u);
}

TEST(linear_create_constructs_object) {
    kairo::LinearAllocator alloc(1024);
    TestObj* obj = alloc.create<TestObj>(99, 1.5f);
    ASSERT_TRUE(obj != nullptr);
    ASSERT_EQ(obj->x, 99);
    ASSERT_NEAR(obj->y, 1.5f, 0.001f);
}

TEST(linear_alignment_is_respected) {
    kairo::LinearAllocator alloc(4096);
    // allocate 1 byte to potentially misalign
    alloc.allocate(1, 1);
    // now allocate with alignment 16
    void* p = alloc.allocate(32, 16);
    ASSERT_TRUE(p != nullptr);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(p) % 16, 0u);
}

// ============================================================
// StackAllocator tests
// ============================================================

TEST(stack_allocate_returns_non_null) {
    kairo::StackAllocator alloc(1024);
    void* p = alloc.allocate(64);
    ASSERT_TRUE(p != nullptr);
}

TEST(stack_get_marker_returns_current_position) {
    kairo::StackAllocator alloc(1024);
    auto m0 = alloc.get_marker();
    ASSERT_EQ(m0, 0u);
    alloc.allocate(32);
    auto m1 = alloc.get_marker();
    ASSERT(m1 >= 32u);
}

TEST(stack_free_to_marker_rolls_back) {
    kairo::StackAllocator alloc(1024);
    auto marker = alloc.get_marker();
    alloc.allocate(64);
    ASSERT(alloc.used() >= 64u);
    alloc.free_to_marker(marker);
    ASSERT_EQ(alloc.used(), 0u);
}

TEST(stack_allocate_after_free_to_marker_reuses_space) {
    kairo::StackAllocator alloc(1024);
    auto marker = alloc.get_marker();
    void* first = alloc.allocate(32);
    alloc.free_to_marker(marker);
    void* second = alloc.allocate(32);
    // should reuse same memory region
    ASSERT_EQ(first, second);
}

TEST(stack_used_tracks_correctly) {
    kairo::StackAllocator alloc(1024);
    ASSERT_EQ(alloc.used(), 0u);
    alloc.allocate(16);
    size_t after_first = alloc.used();
    ASSERT(after_first >= 16u);
    alloc.allocate(16);
    ASSERT(alloc.used() > after_first);
}

TEST(stack_reset_clears_everything) {
    kairo::StackAllocator alloc(1024);
    alloc.allocate(64);
    alloc.allocate(64);
    alloc.reset();
    ASSERT_EQ(alloc.used(), 0u);
}

TEST(stack_nested_marker_free_pattern) {
    kairo::StackAllocator alloc(4096);

    // outer push
    auto outer_marker = alloc.get_marker();
    alloc.allocate(64);
    size_t after_outer = alloc.used();

    // inner push
    auto inner_marker = alloc.get_marker();
    alloc.allocate(128);
    ASSERT(alloc.used() > after_outer);

    // inner pop
    alloc.free_to_marker(inner_marker);
    ASSERT_EQ(alloc.used(), after_outer);

    // outer pop
    alloc.free_to_marker(outer_marker);
    ASSERT_EQ(alloc.used(), 0u);
}
