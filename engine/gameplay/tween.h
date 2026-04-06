#pragma once

#include "core/types.h"
#include "math/math_utils.h"
#include <vector>
#include <functional>
#include <cmath>

namespace kairo {

// easing functions
namespace ease {
    inline float linear(float t) { return t; }
    inline float in_quad(float t) { return t * t; }
    inline float out_quad(float t) { return t * (2.0f - t); }
    inline float in_out_quad(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }
    inline float in_cubic(float t) { return t * t * t; }
    inline float out_cubic(float t) { float u = t - 1.0f; return u * u * u + 1.0f; }
    inline float in_out_cubic(float t) {
        return t < 0.5f ? 4.0f * t * t * t : 1.0f + (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f);
    }
    inline float in_elastic(float t) {
        if (t == 0 || t == 1) return t;
        return -std::pow(2.0f, 10.0f * (t - 1.0f)) * std::sin((t - 1.1f) * 5.0f * PI);
    }
    inline float out_elastic(float t) {
        if (t == 0 || t == 1) return t;
        return std::pow(2.0f, -10.0f * t) * std::sin((t - 0.1f) * 5.0f * PI) + 1.0f;
    }
    inline float out_bounce(float t) {
        if (t < 1.0f / 2.75f) return 7.5625f * t * t;
        if (t < 2.0f / 2.75f) { t -= 1.5f / 2.75f; return 7.5625f * t * t + 0.75f; }
        if (t < 2.5f / 2.75f) { t -= 2.25f / 2.75f; return 7.5625f * t * t + 0.9375f; }
        t -= 2.625f / 2.75f; return 7.5625f * t * t + 0.984375f;
    }
} // namespace ease

using EaseFunc = float(*)(float);

// tween a float value from start to end over duration
class TweenManager {
public:
    using OnComplete = std::function<void()>;

    // tween a float pointer from its current value to `target`
    void tween(float* value, float target, float duration,
               EaseFunc easing = ease::out_quad, OnComplete on_complete = nullptr) {
        m_tweens.push_back({ value, *value, target, duration, 0.0f, easing, std::move(on_complete) });
    }

    void update(float dt) {
        for (size_t i = 0; i < m_tweens.size();) {
            auto& tw = m_tweens[i];
            tw.elapsed += dt;
            float t = clamp(tw.elapsed / tw.duration, 0.0f, 1.0f);
            float eased = tw.easing(t);

            *tw.value = lerp(tw.start, tw.target, eased);

            if (t >= 1.0f) {
                *tw.value = tw.target;
                if (tw.on_complete) tw.on_complete();
                // swap-remove
                m_tweens[i] = std::move(m_tweens.back());
                m_tweens.pop_back();
            } else {
                i++;
            }
        }
    }

    void clear() { m_tweens.clear(); }
    size_t active_count() const { return m_tweens.size(); }

private:
    struct Tween {
        float* value;
        float start;
        float target;
        float duration;
        float elapsed;
        EaseFunc easing;
        OnComplete on_complete;
    };

    std::vector<Tween> m_tweens;
};

} // namespace kairo
