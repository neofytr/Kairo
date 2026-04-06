#pragma once

#include "math/vec3.h"
#include "math/vec4.h"
#include <cmath>
#include <cstring>

namespace kairo {

// column-major 4x4 matrix, matching OpenGL conventions
// m[col][row] — so m[0] is the first column
struct Mat4 {
    float m[4][4];

    Mat4() { std::memset(m, 0, sizeof(m)); }

    static Mat4 identity() {
        Mat4 result;
        result.m[0][0] = 1.0f;
        result.m[1][1] = 1.0f;
        result.m[2][2] = 1.0f;
        result.m[3][3] = 1.0f;
        return result;
    }

    static Mat4 translate(const Vec3& t) {
        Mat4 result = identity();
        result.m[3][0] = t.x;
        result.m[3][1] = t.y;
        result.m[3][2] = t.z;
        return result;
    }

    static Mat4 scale(const Vec3& s) {
        Mat4 result;
        result.m[0][0] = s.x;
        result.m[1][1] = s.y;
        result.m[2][2] = s.z;
        result.m[3][3] = 1.0f;
        return result;
    }

    // rotation around z axis (primary rotation for 2D)
    static Mat4 rotate_z(float radians) {
        Mat4 result = identity();
        float c = std::cos(radians);
        float s = std::sin(radians);
        result.m[0][0] =  c;
        result.m[0][1] =  s;
        result.m[1][0] = -s;
        result.m[1][1] =  c;
        return result;
    }

    // rotation around arbitrary axis (needed when extending to 3D)
    static Mat4 rotate(float radians, const Vec3& axis) {
        Vec3 a = axis.normalized();
        float c = std::cos(radians);
        float s = std::sin(radians);
        float t = 1.0f - c;

        Mat4 result = identity();
        result.m[0][0] = t * a.x * a.x + c;
        result.m[0][1] = t * a.x * a.y + s * a.z;
        result.m[0][2] = t * a.x * a.z - s * a.y;
        result.m[1][0] = t * a.x * a.y - s * a.z;
        result.m[1][1] = t * a.y * a.y + c;
        result.m[1][2] = t * a.y * a.z + s * a.x;
        result.m[2][0] = t * a.x * a.z + s * a.y;
        result.m[2][1] = t * a.y * a.z - s * a.x;
        result.m[2][2] = t * a.z * a.z + c;
        return result;
    }

    // orthographic projection — the primary projection for 2D
    static Mat4 ortho(float left, float right, float bottom, float top, float near_plane, float far_plane) {
        Mat4 result;
        result.m[0][0] =  2.0f / (right - left);
        result.m[1][1] =  2.0f / (top - bottom);
        result.m[2][2] = -2.0f / (far_plane - near_plane);
        result.m[3][0] = -(right + left) / (right - left);
        result.m[3][1] = -(top + bottom) / (top - bottom);
        result.m[3][2] = -(far_plane + near_plane) / (far_plane - near_plane);
        result.m[3][3] =  1.0f;
        return result;
    }

    // perspective projection — for future 3D support
    static Mat4 perspective(float fov_radians, float aspect, float near_plane, float far_plane) {
        float tan_half = std::tan(fov_radians / 2.0f);
        Mat4 result;
        result.m[0][0] = 1.0f / (aspect * tan_half);
        result.m[1][1] = 1.0f / tan_half;
        result.m[2][2] = -(far_plane + near_plane) / (far_plane - near_plane);
        result.m[2][3] = -1.0f;
        result.m[3][2] = -(2.0f * far_plane * near_plane) / (far_plane - near_plane);
        return result;
    }

    // look-at view matrix
    static Mat4 look_at(const Vec3& eye, const Vec3& target, const Vec3& up) {
        Vec3 f = (target - eye).normalized();
        Vec3 r = f.cross(up).normalized();
        Vec3 u = r.cross(f);

        Mat4 result = identity();
        result.m[0][0] =  r.x;
        result.m[1][0] =  r.y;
        result.m[2][0] =  r.z;
        result.m[0][1] =  u.x;
        result.m[1][1] =  u.y;
        result.m[2][1] =  u.z;
        result.m[0][2] = -f.x;
        result.m[1][2] = -f.y;
        result.m[2][2] = -f.z;
        result.m[3][0] = -(r.dot(eye));
        result.m[3][1] = -(u.dot(eye));
        result.m[3][2] =  f.dot(eye);
        return result;
    }

    Mat4 operator*(const Mat4& rhs) const {
        Mat4 result;
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                result.m[col][row] =
                    m[0][row] * rhs.m[col][0] +
                    m[1][row] * rhs.m[col][1] +
                    m[2][row] * rhs.m[col][2] +
                    m[3][row] * rhs.m[col][3];
            }
        }
        return result;
    }

    // raw pointer for glUniformMatrix4fv
    const float* data() const { return &m[0][0]; }
};

// Vec4 constructor that depends on Vec3
inline Vec4::Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

} // namespace kairo
