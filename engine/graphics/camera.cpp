#include "graphics/camera.h"
#include "math/math_utils.h"

namespace kairo {

void Camera::set_orthographic(float width, float height, float near_plane, float far_plane) {
    float hw = width / 2.0f;
    float hh = height / 2.0f;
    m_projection = Mat4::ortho(-hw, hw, -hh, hh, near_plane, far_plane);
    m_dirty = true;
}

void Camera::set_perspective(float fov_degrees, float aspect, float near_plane, float far_plane) {
    m_projection = Mat4::perspective(radians(fov_degrees), aspect, near_plane, far_plane);
    m_dirty = true;
}

void Camera::set_position(const Vec3& pos) {
    m_position = pos;
    m_dirty = true;
}

void Camera::set_rotation(float z_radians) {
    m_rotation = z_radians;
    m_dirty = true;
}

const Mat4& Camera::get_view_projection() const {
    if (m_dirty) {
        recalculate();
    }
    return m_view_projection;
}

void Camera::recalculate() const {
    // view = inverse of camera transform (translate then rotate)
    Mat4 transform = Mat4::translate(m_position) * Mat4::rotate_z(m_rotation);

    // for ortho 2D, the inverse is just negate translation and transpose rotation
    // but let's keep it general — we'll optimize later if profiling demands it
    Mat4 view = Mat4::translate(Vec3(-m_position.x, -m_position.y, -m_position.z))
              * Mat4::rotate_z(-m_rotation);

    // note: for a proper inverse we'd need the full inverse,
    // but for translation + z-rotation this is correct
    m_view_projection = m_projection * view;
    m_dirty = false;
}

} // namespace kairo
