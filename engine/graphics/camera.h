#pragma once

#include "math/mat4.h"
#include "math/vec3.h"

namespace kairo {

class Camera {
public:
    Camera() = default;

    // set up orthographic projection (2D default)
    void set_orthographic(float width, float height, float near_plane = -1.0f, float far_plane = 1.0f);

    // set up perspective projection (3D ready)
    void set_perspective(float fov_degrees, float aspect, float near_plane = 0.1f, float far_plane = 100.0f);

    void set_position(const Vec3& pos);
    void set_rotation(float z_radians);

    const Vec3& get_position() const { return m_position; }
    const Mat4& get_view_projection() const;

private:
    void recalculate() const;

    Vec3 m_position = { 0.0f, 0.0f, 0.0f };
    float m_rotation = 0.0f; // z-axis rotation for 2D

    Mat4 m_projection;

    // cached, recalculated on demand
    mutable Mat4 m_view_projection;
    mutable bool m_dirty = true;
};

} // namespace kairo
