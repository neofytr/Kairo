#pragma once

#include "core/types.h"

namespace kairo {

class Time {
public:
    // call once at startup
    void init();

    // call at the start of each frame
    void update();

    // time since last frame (variable, for rendering/animation)
    float delta_time() const { return m_dt; }

    // fixed timestep for physics (default 60Hz)
    float fixed_delta_time() const { return m_fixed_dt; }
    void set_fixed_delta_time(float dt) { m_fixed_dt = dt; }

    // how much time has accumulated for fixed updates this frame
    // the engine loop consumes this in fixed_dt chunks
    float accumulator() const { return m_accumulator; }
    void consume_accumulator(float amount) { m_accumulator -= amount; }

    // total elapsed time since init
    float elapsed() const { return m_elapsed; }

    // current FPS (smoothed)
    float fps() const { return m_fps; }

private:
    f64 m_last_time = 0.0;
    float m_dt = 0.0f;
    float m_fixed_dt = 1.0f / 60.0f;
    float m_accumulator = 0.0f;
    float m_elapsed = 0.0f;

    // fps tracking
    float m_fps = 0.0f;
    float m_fps_timer = 0.0f;
    int m_frame_count = 0;
};

} // namespace kairo
