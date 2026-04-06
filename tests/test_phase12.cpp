#include "test_framework.h"
#include "math/simd.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "core/job_system.h"
#include "core/network.h"
#include "graphics/frame_graph.h"

#include <atomic>
#include <mutex>
#include <vector>
#include <cstring>

using namespace kairo;

// ============================================================
// SIMD math tests
// ============================================================

TEST(simd_add_matches_vec4_operator_plus) {
    Vec4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4 b(5.0f, 6.0f, 7.0f, 8.0f);
    Vec4 expected = a + b;
    Vec4 result = simd::add(a, b);
    ASSERT_NEAR(result.x, expected.x, 1e-5f);
    ASSERT_NEAR(result.y, expected.y, 1e-5f);
    ASSERT_NEAR(result.z, expected.z, 1e-5f);
    ASSERT_NEAR(result.w, expected.w, 1e-5f);
}

TEST(simd_sub_matches_vec4_operator_minus) {
    Vec4 a(10.0f, 20.0f, 30.0f, 40.0f);
    Vec4 b(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4 expected = a - b;
    Vec4 result = simd::sub(a, b);
    ASSERT_NEAR(result.x, expected.x, 1e-5f);
    ASSERT_NEAR(result.y, expected.y, 1e-5f);
    ASSERT_NEAR(result.z, expected.z, 1e-5f);
    ASSERT_NEAR(result.w, expected.w, 1e-5f);
}

TEST(simd_mul_matches_vec4_operator_times) {
    Vec4 a(2.0f, 3.0f, 4.0f, 5.0f);
    float s = 3.0f;
    Vec4 expected = a * s;
    Vec4 result = simd::mul(a, s);
    ASSERT_NEAR(result.x, expected.x, 1e-5f);
    ASSERT_NEAR(result.y, expected.y, 1e-5f);
    ASSERT_NEAR(result.z, expected.z, 1e-5f);
    ASSERT_NEAR(result.w, expected.w, 1e-5f);
}

TEST(simd_dot_matches_vec4_dot) {
    Vec4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4 b(5.0f, 6.0f, 7.0f, 8.0f);
    float expected = a.dot(b);
    float result = simd::dot(a, b);
    ASSERT_NEAR(result, expected, 1e-4f);
}

TEST(simd_mat4_multiply_matches_mat4_operator) {
    Mat4 a = Mat4::scale(Vec3(2.0f, 3.0f, 4.0f));
    Mat4 b = Mat4::translate(Vec3(1.0f, 2.0f, 3.0f));
    Mat4 expected = a * b;
    Mat4 result = simd::mat4_multiply(a, b);
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            ASSERT_NEAR(result.m[col][row], expected.m[col][row], 1e-5f);
        }
    }
}

TEST(simd_mat4_multiply_identity_returns_same) {
    Mat4 m = Mat4::translate(Vec3(5.0f, 10.0f, 15.0f));
    Mat4 id = Mat4::identity();
    Mat4 result = simd::mat4_multiply(m, id);
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            ASSERT_NEAR(result.m[col][row], m.m[col][row], 1e-5f);
        }
    }
}

TEST(simd_mat4_transform_identity_returns_same_vector) {
    Mat4 id = Mat4::identity();
    Vec4 v(3.0f, 7.0f, 11.0f, 1.0f);
    Vec4 result = simd::mat4_transform(id, v);
    ASSERT_NEAR(result.x, v.x, 1e-5f);
    ASSERT_NEAR(result.y, v.y, 1e-5f);
    ASSERT_NEAR(result.z, v.z, 1e-5f);
    ASSERT_NEAR(result.w, v.w, 1e-5f);
}

TEST(simd_mat4_transform_translation_moves_point) {
    Mat4 t = Mat4::translate(Vec3(10.0f, 20.0f, 30.0f));
    Vec4 point(1.0f, 2.0f, 3.0f, 1.0f); // w=1 for point
    Vec4 result = simd::mat4_transform(t, point);
    ASSERT_NEAR(result.x, 11.0f, 1e-5f);
    ASSERT_NEAR(result.y, 22.0f, 1e-5f);
    ASSERT_NEAR(result.z, 33.0f, 1e-5f);
    ASSERT_NEAR(result.w, 1.0f, 1e-5f);
}

// ============================================================
// Job System tests
// ============================================================

TEST(job_system_init_and_shutdown) {
    JobSystem js;
    js.init(2);
    ASSERT_EQ(js.thread_count(), (u32)2);
    js.shutdown();
    // After shutdown, thread_count should be 0 (workers joined)
    ASSERT_EQ(js.thread_count(), (u32)0);
}

TEST(job_submit_executes) {
    JobSystem js;
    js.init(2);
    std::atomic<int> counter{0};
    auto f = js.submit([&]() { counter++; });
    f.wait();
    ASSERT_EQ(counter.load(), 1);
    js.shutdown();
}

TEST(job_submit_returns_future_that_completes) {
    JobSystem js;
    js.init(2);
    std::atomic<bool> done{false};
    auto f = js.submit([&]() { done.store(true); });
    f.wait();
    ASSERT_TRUE(done.load());
    js.shutdown();
}

TEST(job_parallel_for_processes_all_indices) {
    JobSystem js;
    js.init(4);
    const u32 count = 100;
    std::atomic<int> sum{0};
    js.parallel_for(count, [&](u32 index) {
        sum.fetch_add(static_cast<int>(index));
    });
    // sum of 0..99 = 4950
    ASSERT_EQ(sum.load(), 4950);
    js.shutdown();
}

TEST(job_parallel_for_large_count) {
    JobSystem js;
    js.init(4);
    const u32 count = 1000;
    std::vector<std::atomic<int>> flags(count);
    for (u32 i = 0; i < count; i++) {
        flags[i].store(0);
    }
    js.parallel_for(count, [&](u32 index) {
        flags[index].store(1);
    });
    int processed = 0;
    for (u32 i = 0; i < count; i++) {
        if (flags[i].load() == 1) processed++;
    }
    ASSERT_EQ(processed, (int)count);
    js.shutdown();
}

TEST(job_wait_idle_blocks_until_done) {
    JobSystem js;
    js.init(2);
    std::atomic<int> counter{0};
    for (int i = 0; i < 50; i++) {
        js.submit([&]() { counter++; });
    }
    js.wait_idle();
    ASSERT_EQ(counter.load(), 50);
    js.shutdown();
}

// ============================================================
// Network serialization tests
// ============================================================

TEST(network_serialize_deserialize_roundtrip) {
    NetEntityState original;
    original.entity_id = 42;
    original.position = Vec2(100.5f, 200.5f);
    original.velocity = Vec2(-3.0f, 7.5f);
    original.rotation = 1.57f;

    std::vector<u8> buffer;
    serialize_state(original, buffer);

    NetEntityState restored;
    bool ok = deserialize_state(buffer.data(), buffer.size(), restored);

    ASSERT_TRUE(ok);
    ASSERT_EQ(restored.entity_id, original.entity_id);
    ASSERT_NEAR(restored.position.x, original.position.x, 1e-5f);
    ASSERT_NEAR(restored.position.y, original.position.y, 1e-5f);
    ASSERT_NEAR(restored.velocity.x, original.velocity.x, 1e-5f);
    ASSERT_NEAR(restored.velocity.y, original.velocity.y, 1e-5f);
    ASSERT_NEAR(restored.rotation, original.rotation, 1e-5f);
}

TEST(network_deserialize_wrong_size_returns_false) {
    u8 small_buffer[4] = {0, 0, 0, 0};
    NetEntityState state;
    bool ok = deserialize_state(small_buffer, 4, state);
    ASSERT_FALSE(ok);
}

TEST(network_serialize_produces_expected_byte_count) {
    // entity_id (u32=4) + position (2 floats=8) + velocity (2 floats=8) + rotation (float=4) = 24
    NetEntityState state;
    state.entity_id = 1;
    state.position = Vec2(0.0f, 0.0f);
    state.velocity = Vec2(0.0f, 0.0f);
    state.rotation = 0.0f;

    std::vector<u8> buffer;
    serialize_state(state, buffer);
    ASSERT_EQ((int)buffer.size(), 24);
}

TEST(network_multiple_serialize_deserialize_consistent) {
    NetEntityState state;
    state.entity_id = 999;
    state.position = Vec2(55.5f, -12.3f);
    state.velocity = Vec2(1.0f, -1.0f);
    state.rotation = 3.14159f;

    // Serialize twice and verify both produce identical bytes
    std::vector<u8> buf1, buf2;
    serialize_state(state, buf1);
    serialize_state(state, buf2);
    ASSERT_EQ(buf1.size(), buf2.size());
    bool identical = (std::memcmp(buf1.data(), buf2.data(), buf1.size()) == 0);
    ASSERT_TRUE(identical);

    // Deserialize both and verify identical results
    NetEntityState r1, r2;
    ASSERT_TRUE(deserialize_state(buf1.data(), buf1.size(), r1));
    ASSERT_TRUE(deserialize_state(buf2.data(), buf2.size(), r2));
    ASSERT_EQ(r1.entity_id, r2.entity_id);
    ASSERT_NEAR(r1.position.x, r2.position.x, 1e-5f);
    ASSERT_NEAR(r1.position.y, r2.position.y, 1e-5f);
    ASSERT_NEAR(r1.rotation, r2.rotation, 1e-5f);
}

// ============================================================
// Frame Graph tests
// ============================================================

TEST(frame_graph_add_pass_increases_count) {
    FrameGraph fg;
    ASSERT_EQ(fg.pass_count(), (size_t)0);

    RenderPass pass;
    pass.name = "geometry";
    pass.output = "g_buffer";
    pass.execute = [](const std::unordered_map<std::string, u32>&) {};
    fg.add_pass(pass);

    ASSERT_EQ(fg.pass_count(), (size_t)1);
}

TEST(frame_graph_compile_no_passes_succeeds) {
    FrameGraph fg;
    // compile with no passes should succeed (nothing to validate)
    bool ok = fg.compile();
    ASSERT_TRUE(ok);
}

TEST(frame_graph_add_screen_pass_works) {
    FrameGraph fg;
    fg.add_screen_pass("final", {}, [](const std::unordered_map<std::string, u32>&) {});
    ASSERT_EQ(fg.pass_count(), (size_t)1);

    const auto& passes = fg.get_passes();
    ASSERT_EQ(passes[0].name, std::string("final"));
    // screen pass should have empty output
    ASSERT_TRUE(passes[0].output.empty());
}

TEST(frame_graph_get_passes_returns_correct_count) {
    FrameGraph fg;

    RenderPass p1;
    p1.name = "shadow";
    p1.output = "shadow_map";
    p1.execute = [](const std::unordered_map<std::string, u32>&) {};
    fg.add_pass(p1);

    RenderPass p2;
    p2.name = "lighting";
    p2.inputs = {"shadow_map"};
    p2.output = "lit_scene";
    p2.execute = [](const std::unordered_map<std::string, u32>&) {};
    fg.add_pass(p2);

    fg.add_screen_pass("composite", {"lit_scene"}, [](const std::unordered_map<std::string, u32>&) {});

    ASSERT_EQ(fg.pass_count(), (size_t)3);

    const auto& passes = fg.get_passes();
    ASSERT_EQ(passes.size(), (size_t)3);
    ASSERT_EQ(passes[0].name, std::string("shadow"));
    ASSERT_EQ(passes[1].name, std::string("lighting"));
    ASSERT_EQ(passes[2].name, std::string("composite"));
}
