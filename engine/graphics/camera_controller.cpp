#include "graphics/camera_controller.h"
#include <cmath>

namespace kairo {

void CameraController::update(float dt) {
    if (!m_camera) return;

    // dead zone — only move if target is outside the dead zone
    Vec2 diff = m_target - m_current;
    Vec2 move = { 0, 0 };

    if (std::abs(diff.x) > m_dead_zone.x) {
        float sign = diff.x > 0 ? 1.0f : -1.0f;
        move.x = diff.x - sign * m_dead_zone.x;
    }
    if (std::abs(diff.y) > m_dead_zone.y) {
        float sign = diff.y > 0 ? 1.0f : -1.0f;
        move.y = diff.y - sign * m_dead_zone.y;
    }

    // smooth interpolation toward target
    float t = 1.0f - std::exp(-m_smoothing * dt);
    m_current.x += move.x * t;
    m_current.y += move.y * t;

    // apply zoom to ortho projection
    // base size is 1280x720, scaled by zoom
    m_camera->set_orthographic(1280.0f * m_zoom, 720.0f * m_zoom);

    // compute shake offset
    Vec2 shake_offset = { 0, 0 };
    float shake_rotation = 0.0f;

    if (m_trauma > 0.0f) {
        // shake intensity is trauma squared (feels better than linear)
        float shake = m_trauma * m_trauma;
        m_shake_time += dt;

        shake_offset.x = rand_range(-1.0f, 1.0f) * m_max_shake_offset * shake;
        shake_offset.y = rand_range(-1.0f, 1.0f) * m_max_shake_offset * shake;
        shake_rotation = rand_range(-1.0f, 1.0f) * m_max_shake_rotation * shake;

        // decay trauma over time
        m_trauma = std::max(0.0f, m_trauma - m_shake_decay * dt);
    }

    // set final camera position
    m_camera->set_position(Vec3(
        m_current.x + shake_offset.x,
        m_current.y + shake_offset.y,
        0.0f
    ));
    m_camera->set_rotation(shake_rotation);
}

} // namespace kairo
