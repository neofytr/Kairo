#pragma once

#include <cmath>

namespace kairo {

struct Vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    Vec4() = default;
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    explicit Vec4(float scalar) : x(scalar), y(scalar), z(scalar), w(scalar) {}

    // convenience: construct from vec3 + w (useful for positions vs directions)
    Vec4(const struct Vec3& v, float w);

    Vec4 operator+(const Vec4& rhs) const { return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w }; }
    Vec4 operator-(const Vec4& rhs) const { return { x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w }; }
    Vec4 operator*(float s) const { return { x * s, y * s, z * s, w * s }; }

    float dot(const Vec4& rhs) const {
        return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
    }
};

inline Vec4 operator*(float s, const Vec4& v) { return v * s; }

} // namespace kairo
