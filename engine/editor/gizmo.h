#pragma once

#include "core/types.h"
#include "math/vec2.h"
#include "ecs/entity.h"

namespace kairo {

class Camera;

enum class GizmoMode { Translate, Rotate, Scale };

class Gizmo {
public:
    void set_mode(GizmoMode mode) { m_mode = mode; }
    GizmoMode get_mode() const { return m_mode; }

    // call each frame with the selected entity's position
    // returns true if the gizmo is being actively dragged
    // delta_out receives the movement delta to apply to the entity
    bool update(const Vec2& entity_pos, const Vec2& mouse_world_pos,
                bool mouse_down, bool mouse_just_pressed, Vec2& delta_out);

    // draw the gizmo overlay
    void draw(const Vec2& entity_pos, const Camera& camera);

    bool is_active() const { return m_dragging; }

private:
    GizmoMode m_mode = GizmoMode::Translate;
    bool m_dragging = false;
    int m_active_axis = -1;  // 0=x, 1=y, -1=none (free drag)
    Vec2 m_drag_start = {};
    Vec2 m_drag_offset = {};

    float m_handle_length = 50.0f;
    float m_handle_grab_radius = 12.0f;

    // rotate mode state
    float m_drag_start_angle = 0.0f;
};

} // namespace kairo
