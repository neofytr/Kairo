#pragma once

#include "core/types.h"
#include <string>

struct GLFWwindow;

namespace kairo {

struct WindowConfig {
    std::string title = "Kairo Engine";
    i32 width  = 1280;
    i32 height = 720;
    bool vsync = true;
};

class Window {
public:
    Window() = default;
    ~Window();

    bool init(const WindowConfig& config);
    void shutdown();

    void poll_events();
    void swap_buffers();
    bool should_close() const;

    i32 get_width() const { return m_width; }
    i32 get_height() const { return m_height; }
    f32 get_aspect_ratio() const { return static_cast<f32>(m_width) / m_height; }

    GLFWwindow* get_native() const { return m_window; }

private:
    GLFWwindow* m_window = nullptr;
    i32 m_width  = 0;
    i32 m_height = 0;
};

} // namespace kairo
