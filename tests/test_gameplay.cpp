#include "test_framework.h"
#include "gameplay/timer.h"
#include "gameplay/tween.h"

// ============================================================
// TimerManager tests
// ============================================================

TEST(timer_after_fires_callback_after_delay) {
    kairo::TimerManager tm;
    int count = 0;
    tm.after(1.0f, [&]() { count++; });
    tm.update(1.0f);
    ASSERT_EQ(count, 1);
}

TEST(timer_after_does_not_fire_before_delay) {
    kairo::TimerManager tm;
    int count = 0;
    tm.after(1.0f, [&]() { count++; });
    tm.update(0.5f);
    ASSERT_EQ(count, 0);
}

TEST(timer_callback_fires_exactly_once) {
    kairo::TimerManager tm;
    int count = 0;
    tm.after(1.0f, [&]() { count++; });
    tm.update(1.0f);
    tm.update(1.0f);
    tm.update(1.0f);
    ASSERT_EQ(count, 1);
}

TEST(timer_every_fires_repeatedly) {
    kairo::TimerManager tm;
    int count = 0;
    tm.every(1.0f, [&]() { count++; });
    tm.update(1.0f);
    ASSERT_EQ(count, 1);
    tm.update(1.0f);
    ASSERT_EQ(count, 2);
}

TEST(timer_every_fires_correct_number_of_times) {
    kairo::TimerManager tm;
    int count = 0;
    tm.every(0.5f, [&]() { count++; });
    // 3 seconds at 0.5 interval = 6 fires
    for (int i = 0; i < 6; i++) {
        tm.update(0.5f);
    }
    ASSERT_EQ(count, 6);
}

TEST(timer_zero_dt_does_not_fire) {
    kairo::TimerManager tm;
    int count = 0;
    tm.after(1.0f, [&]() { count++; });
    tm.update(0.0f);
    ASSERT_EQ(count, 0);
}

TEST(timer_clear_removes_all) {
    kairo::TimerManager tm;
    int count = 0;
    tm.after(1.0f, [&]() { count++; });
    tm.every(1.0f, [&]() { count++; });
    tm.clear();
    tm.update(2.0f);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(tm.active_count(), 0u);
}

TEST(timer_active_count_tracks_correctly) {
    kairo::TimerManager tm;
    ASSERT_EQ(tm.active_count(), 0u);
    tm.after(1.0f, []() {});
    ASSERT_EQ(tm.active_count(), 1u);
    tm.after(2.0f, []() {});
    ASSERT_EQ(tm.active_count(), 2u);
    tm.update(1.5f); // first timer fires and is removed
    ASSERT_EQ(tm.active_count(), 1u);
}

TEST(timer_multiple_timers_fire_in_correct_order) {
    kairo::TimerManager tm;
    std::vector<int> order;
    tm.after(0.5f, [&]() { order.push_back(1); });
    tm.after(1.0f, [&]() { order.push_back(2); });
    tm.after(1.5f, [&]() { order.push_back(3); });
    tm.update(0.5f);
    tm.update(0.5f);
    tm.update(0.5f);
    ASSERT_EQ(order.size(), 3u);
    ASSERT_EQ(order[0], 1);
    ASSERT_EQ(order[1], 2);
    ASSERT_EQ(order[2], 3);
}

// ============================================================
// TweenManager tests
// ============================================================

TEST(tween_moves_value_from_start_to_target) {
    kairo::TweenManager tm;
    float val = 0.0f;
    tm.tween(&val, 100.0f, 1.0f, kairo::ease::linear);
    tm.update(1.0f);
    ASSERT_NEAR(val, 100.0f, 0.01f);
}

TEST(tween_at_t0_is_at_start) {
    kairo::TweenManager tm;
    float val = 10.0f;
    tm.tween(&val, 50.0f, 1.0f, kairo::ease::linear);
    // update with 0 dt -- value should stay at start
    tm.update(0.0f);
    ASSERT_NEAR(val, 10.0f, 0.01f);
}

TEST(tween_at_completion_is_exactly_at_target) {
    kairo::TweenManager tm;
    float val = 0.0f;
    tm.tween(&val, 42.0f, 1.0f, kairo::ease::out_quad);
    tm.update(2.0f); // overshoot duration
    ASSERT_NEAR(val, 42.0f, 0.001f);
}

TEST(tween_on_complete_fires_when_done) {
    kairo::TweenManager tm;
    float val = 0.0f;
    bool completed = false;
    tm.tween(&val, 10.0f, 1.0f, kairo::ease::linear, [&]() { completed = true; });
    tm.update(1.0f);
    ASSERT_TRUE(completed);
}

TEST(tween_removed_after_completion) {
    kairo::TweenManager tm;
    float val = 0.0f;
    tm.tween(&val, 10.0f, 1.0f, kairo::ease::linear);
    ASSERT_EQ(tm.active_count(), 1u);
    tm.update(1.0f);
    ASSERT_EQ(tm.active_count(), 0u);
}

TEST(tween_linear_produces_linear_interpolation) {
    kairo::TweenManager tm;
    float val = 0.0f;
    tm.tween(&val, 100.0f, 1.0f, kairo::ease::linear);
    tm.update(0.5f);
    ASSERT_NEAR(val, 50.0f, 0.1f);
}

TEST(tween_in_quad_starts_slow) {
    kairo::TweenManager tm;
    float val = 0.0f;
    tm.tween(&val, 100.0f, 1.0f, kairo::ease::in_quad);
    tm.update(0.25f);
    // in_quad at t=0.25 is 0.0625, so value ~6.25 which is less than linear 25
    ASSERT(val < 25.0f);
}

TEST(tween_out_quad_ends_slow) {
    kairo::TweenManager tm;
    float val = 0.0f;
    tm.tween(&val, 100.0f, 1.0f, kairo::ease::out_quad);
    tm.update(0.25f);
    // out_quad at t=0.25 is 0.4375, so value ~43.75 which is more than linear 25
    ASSERT(val > 25.0f);
}

TEST(tween_out_bounce_bounces_near_end) {
    // out_bounce at t=0.5 should be around 0.765625
    float v = kairo::ease::out_bounce(0.5f);
    ASSERT(v > 0.7f);
    ASSERT(v < 0.8f);
}

TEST(tween_multiple_simultaneous_tweens) {
    kairo::TweenManager tm;
    float a = 0.0f, b = 100.0f;
    tm.tween(&a, 50.0f, 1.0f, kairo::ease::linear);
    tm.tween(&b, 0.0f, 1.0f, kairo::ease::linear);
    ASSERT_EQ(tm.active_count(), 2u);
    tm.update(0.5f);
    ASSERT_NEAR(a, 25.0f, 0.1f);
    ASSERT_NEAR(b, 50.0f, 0.1f);
}

TEST(tween_clear_removes_all) {
    kairo::TweenManager tm;
    float a = 0.0f, b = 0.0f;
    tm.tween(&a, 10.0f, 1.0f, kairo::ease::linear);
    tm.tween(&b, 20.0f, 1.0f, kairo::ease::linear);
    tm.clear();
    ASSERT_EQ(tm.active_count(), 0u);
    tm.update(1.0f);
    // values should not have changed
    ASSERT_NEAR(a, 0.0f, 0.001f);
    ASSERT_NEAR(b, 0.0f, 0.001f);
}

// ============================================================
// Easing function tests
// ============================================================

TEST(easing_all_return_zero_at_t0) {
    ASSERT_NEAR(kairo::ease::linear(0.0f), 0.0f, 1e-6f);
    ASSERT_NEAR(kairo::ease::in_quad(0.0f), 0.0f, 1e-6f);
    ASSERT_NEAR(kairo::ease::out_quad(0.0f), 0.0f, 1e-6f);
    ASSERT_NEAR(kairo::ease::in_out_quad(0.0f), 0.0f, 1e-6f);
    ASSERT_NEAR(kairo::ease::in_cubic(0.0f), 0.0f, 1e-6f);
    ASSERT_NEAR(kairo::ease::out_cubic(0.0f), 0.0f, 1e-6f);
    ASSERT_NEAR(kairo::ease::out_bounce(0.0f), 0.0f, 1e-6f);
    ASSERT_NEAR(kairo::ease::out_elastic(0.0f), 0.0f, 1e-6f);
}

TEST(easing_all_return_one_at_t1) {
    ASSERT_NEAR(kairo::ease::linear(1.0f), 1.0f, 1e-6f);
    ASSERT_NEAR(kairo::ease::in_quad(1.0f), 1.0f, 1e-6f);
    ASSERT_NEAR(kairo::ease::out_quad(1.0f), 1.0f, 1e-6f);
    ASSERT_NEAR(kairo::ease::in_out_quad(1.0f), 1.0f, 1e-6f);
    ASSERT_NEAR(kairo::ease::in_cubic(1.0f), 1.0f, 1e-6f);
    ASSERT_NEAR(kairo::ease::out_cubic(1.0f), 1.0f, 1e-6f);
    ASSERT_NEAR(kairo::ease::out_bounce(1.0f), 1.0f, 1e-6f);
    ASSERT_NEAR(kairo::ease::out_elastic(1.0f), 1.0f, 1e-6f);
}
