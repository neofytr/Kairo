#pragma once

#include "core/types.h"

struct GLFWwindow;

namespace kairo {

// manages ImGui lifecycle — init, new frame, render
class ImGuiLayer {
public:
    static bool init(GLFWwindow* window);
    static void shutdown();

    // call at the start of each frame before any ImGui calls
    static void begin_frame();

    // call after all ImGui calls to render the draw data
    static void end_frame();

    // whether the editor UI is visible (toggled with F1)
    static bool is_visible() { return s_visible; }
    static void set_visible(bool visible) { s_visible = visible; }
    static void toggle() { s_visible = !s_visible; }

private:
    static bool s_visible;
};

} // namespace kairo
