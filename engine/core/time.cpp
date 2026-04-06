#include "core/time.h"
#include <GLFW/glfw3.h>
#include <algorithm>

namespace kairo {

void Time::init() {
    m_last_time = glfwGetTime();
}

void Time::update() {
    f64 now = glfwGetTime();
    m_dt = static_cast<float>(now - m_last_time);
    m_last_time = now;

    // clamp to avoid spiral of death if frame takes too long
    // (e.g. breakpoint, window drag, etc.)
    m_dt = std::min(m_dt, 0.1f);

    m_accumulator += m_dt;
    m_elapsed += m_dt;

    // update FPS counter every half second
    m_frame_count++;
    m_fps_timer += m_dt;
    if (m_fps_timer >= 0.5f) {
        m_fps = m_frame_count / m_fps_timer;
        m_frame_count = 0;
        m_fps_timer = 0.0f;
    }
}

} // namespace kairo
