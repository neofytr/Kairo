#pragma once

#include <cmath>

namespace kairo {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}
    explicit Vec2(float scalar) : x(scalar), y(scalar) {}

    Vec2 operator+(const Vec2& rhs) const { return { x + rhs.x, y + rhs.y }; }
    Vec2 operator-(const Vec2& rhs) const { return { x - rhs.x, y - rhs.y }; }
    Vec2 operator*(float s) const { return { x * s, y * s }; }
    Vec2 operator/(float s) const { return { x / s, y / s }; }
    Vec2 operator-() const { return { -x, -y }; }

    Vec2& operator+=(const Vec2& rhs) { x += rhs.x; y += rhs.y; return *this; }
    Vec2& operator-=(const Vec2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float length() const { return std::sqrt(x * x + y * y); }
    float length_sq() const { return x * x + y * y; }

    Vec2 normalized() const {
        float len = length();
        return len > 0.0f ? *this / len : Vec2{};
    }

    float dot(const Vec2& rhs) const { return x * rhs.x + y * rhs.y; }

    // perpendicular (rotated 90 degrees CCW)
    Vec2 perp() const { return { -y, x }; }
};

inline Vec2 operator*(float s, const Vec2& v) { return v * s; }

} // namespace kairo
