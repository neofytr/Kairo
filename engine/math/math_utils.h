#pragma once

#include <cmath>
#include <algorithm>

namespace kairo {

constexpr float PI = 3.14159265358979323846f;
constexpr float TAU = PI * 2.0f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

inline float radians(float degrees) { return degrees * DEG_TO_RAD; }
inline float degrees(float radians) { return radians * RAD_TO_DEG; }

inline float clamp(float val, float lo, float hi) {
    return std::max(lo, std::min(val, hi));
}

inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// smooth interpolation (ease in/out)
inline float smoothstep(float edge0, float edge1, float x) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// approximately equal with epsilon
inline bool approx_equal(float a, float b, float epsilon = 1e-6f) {
    return std::fabs(a - b) < epsilon;
}

} // namespace kairo
