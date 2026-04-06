#pragma once

#include "graphics/camera.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/math_utils.h"

#include <random>

namespace kairo {

// smooth-follow camera with dead zones, zoom, and trauma-based screen shake
class CameraController {
public:
    CameraController() = default;

    void set_camera(Camera* camera) { m_camera = camera; }

    // follow a target position with smoothing
    void set_target(const Vec2& target) { m_target = target; }

    // how quickly the camera catches up (0 = instant, higher = slower)
    void set_smoothing(float smoothing) { m_smoothing = smoothing; }

    // dead zone — camera doesn't move if target is within this rect of center
    void set_dead_zone(const Vec2& half_size) { m_dead_zone = half_size; }

    // zoom (1.0 = normal, 0.5 = zoomed in 2x, 2.0 = zoomed out 2x)
    void set_zoom(float zoom) { m_zoom = clamp(zoom, 0.1f, 10.0f); }
    float get_zoom() const { return m_zoom; }

    // screen shake — add trauma (0-1), it decays over time
    // trauma is squared for the actual shake intensity (feels more natural)
    void add_trauma(float amount) { m_trauma = clamp(m_trauma + amount, 0.0f, 1.0f); }
    void set_shake_intensity(float max_offset, float max_rotation) {
        m_max_shake_offset = max_offset;
        m_max_shake_rotation = max_rotation;
    }
    void set_shake_decay(float rate) { m_shake_decay = rate; }

    // call each frame with dt
    void update(float dt);

    // get the actual camera position after all offsets
    Vec2 get_position() const { return { m_current.x, m_current.y }; }

private:
    Camera* m_camera = nullptr;
    Vec2 m_target = { 0, 0 };
    Vec2 m_current = { 0, 0 };

    float m_smoothing = 5.0f;
    Vec2 m_dead_zone = { 0, 0 };
    float m_zoom = 1.0f;

    // shake
    float m_trauma = 0.0f;
    float m_shake_decay = 2.0f;
    float m_max_shake_offset = 15.0f;
    float m_max_shake_rotation = 0.05f;
    float m_shake_time = 0.0f;

    std::mt19937 m_rng{ 42 };

    float rand_range(float lo, float hi) {
        std::uniform_real_distribution<float> d(lo, hi);
        return d(m_rng);
    }
};

} // namespace kairo
