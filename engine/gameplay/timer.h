#pragma once

#include "core/types.h"
#include <vector>
#include <functional>

namespace kairo {

// simple timer system — schedule callbacks to run after a delay
class TimerManager {
public:
    using Callback = std::function<void()>;

    // run callback after `delay` seconds
    void after(float delay, Callback callback) {
        m_timers.push_back({ delay, 0.0f, std::move(callback), false });
    }

    // run callback every `interval` seconds
    void every(float interval, Callback callback) {
        m_timers.push_back({ interval, 0.0f, std::move(callback), true });
    }

    void update(float dt) {
        for (size_t i = 0; i < m_timers.size();) {
            auto& t = m_timers[i];
            t.elapsed += dt;

            if (t.elapsed >= t.duration) {
                t.callback();

                if (t.repeating) {
                    t.elapsed -= t.duration;
                    i++;
                } else {
                    // swap-remove
                    m_timers[i] = std::move(m_timers.back());
                    m_timers.pop_back();
                }
            } else {
                i++;
            }
        }
    }

    void clear() { m_timers.clear(); }
    size_t active_count() const { return m_timers.size(); }

private:
    struct Timer {
        float duration;
        float elapsed;
        Callback callback;
        bool repeating;
    };

    std::vector<Timer> m_timers;
};

} // namespace kairo
