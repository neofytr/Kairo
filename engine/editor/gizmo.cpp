#include "editor/gizmo.h"
#include "graphics/debug_draw.h"
#include "graphics/camera.h"
#include "math/math_utils.h"

#include <cmath>

namespace kairo {

// helper: angle of a vector relative to origin
static float vec_angle(const Vec2& v) {
    return std::atan2(v.y, v.x);
}

bool Gizmo::update(const Vec2& entity_pos, const Vec2& mouse_world_pos,
                   bool mouse_down, bool mouse_just_pressed, Vec2& delta_out) {
    delta_out = {};

    if (mouse_just_pressed && mouse_down) {
        Vec2 rel = mouse_world_pos - entity_pos;

        if (m_mode == GizmoMode::Translate || m_mode == GizmoMode::Scale) {
            // check X axis handle tip
            Vec2 x_tip = Vec2(m_handle_length, 0.0f);
            // check Y axis handle tip
            Vec2 y_tip = Vec2(0.0f, m_handle_length);

            float dx = (rel - x_tip).length();
            float dy = (rel - y_tip).length();
            float dc = rel.length(); // distance to center

            if (dx <= m_handle_grab_radius) {
                m_active_axis = 0;
                m_dragging = true;
            } else if (dy <= m_handle_grab_radius) {
                m_active_axis = 1;
                m_dragging = true;
            } else if (dc <= m_handle_grab_radius) {
                m_active_axis = -1; // free drag
                m_dragging = true;
            }

            if (m_dragging) {
                m_drag_start = mouse_world_pos;
                m_drag_offset = rel;
            }
        } else if (m_mode == GizmoMode::Rotate) {
            // check if mouse is near the rotation circle
            float dist = rel.length();
            float ring_dist = std::fabs(dist - m_handle_length);
            if (ring_dist <= m_handle_grab_radius) {
                m_dragging = true;
                m_active_axis = -1;
                m_drag_start = mouse_world_pos;
                m_drag_start_angle = vec_angle(rel);
            }
        }
    }

    if (!mouse_down) {
        m_dragging = false;
        m_active_axis = -1;
        return false;
    }

    if (!m_dragging)
        return false;

    if (m_mode == GizmoMode::Translate) {
        Vec2 raw_delta = mouse_world_pos - m_drag_start;
        // constrain to axis if one is active
        if (m_active_axis == 0)
            raw_delta.y = 0.0f;
        else if (m_active_axis == 1)
            raw_delta.x = 0.0f;

        delta_out = raw_delta;
        m_drag_start = mouse_world_pos; // continuous delta
        return true;
    }

    if (m_mode == GizmoMode::Scale) {
        Vec2 raw_delta = mouse_world_pos - m_drag_start;
        if (m_active_axis == 0)
            raw_delta.y = 0.0f;
        else if (m_active_axis == 1)
            raw_delta.x = 0.0f;

        // scale mode: delta is multiplicative (scale factor encoded as delta)
        delta_out = raw_delta * 0.01f;
        m_drag_start = mouse_world_pos;
        return true;
    }

    if (m_mode == GizmoMode::Rotate) {
        Vec2 rel = mouse_world_pos - entity_pos;
        float current_angle = vec_angle(rel);
        float angle_delta = current_angle - m_drag_start_angle;

        // wrap to [-PI, PI]
        if (angle_delta > PI) angle_delta -= TAU;
        if (angle_delta < -PI) angle_delta += TAU;

        delta_out = Vec2(angle_delta, 0.0f); // angle stored in x
        m_drag_start_angle = current_angle;
        return true;
    }

    return false;
}

void Gizmo::draw(const Vec2& entity_pos, const Camera& camera) {
    (void)camera;

    const Vec4 red    = { 1.0f, 0.2f, 0.2f, 1.0f };
    const Vec4 green  = { 0.2f, 1.0f, 0.2f, 1.0f };
    const Vec4 white  = { 1.0f, 1.0f, 1.0f, 1.0f };
    const Vec4 yellow = { 1.0f, 1.0f, 0.2f, 1.0f };

    const Vec4 active_red   = (m_dragging && m_active_axis == 0) ? yellow : red;
    const Vec4 active_green = (m_dragging && m_active_axis == 1) ? yellow : green;
    const Vec4 active_white = (m_dragging && m_active_axis == -1) ? yellow : white;

    if (m_mode == GizmoMode::Translate || m_mode == GizmoMode::Scale) {
        Vec2 x_end = entity_pos + Vec2(m_handle_length, 0.0f);
        Vec2 y_end = entity_pos + Vec2(0.0f, m_handle_length);

        // axis lines
        DebugDraw::line(entity_pos, x_end, active_red);
        DebugDraw::line(entity_pos, y_end, active_green);

        // handle tip squares
        float tip_size = 4.0f;
        DebugDraw::rect(x_end, Vec2(tip_size, tip_size), active_red);
        DebugDraw::rect(y_end, Vec2(tip_size, tip_size), active_green);

        // center square
        DebugDraw::rect(entity_pos, Vec2(tip_size, tip_size), active_white);
    }

    if (m_mode == GizmoMode::Rotate) {
        // rotation ring
        Vec4 ring_color = m_dragging ? yellow : white;
        DebugDraw::circle(entity_pos, m_handle_length, ring_color, 32);

        // small center dot
        DebugDraw::rect(entity_pos, Vec2(3.0f, 3.0f), white);
    }
}

} // namespace kairo
