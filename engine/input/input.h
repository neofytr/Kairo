#pragma once

#include "core/types.h"
#include "math/vec2.h"

struct GLFWwindow;

namespace kairo {

// key codes — maps directly to GLFW key values
// only defining the ones we commonly need, add more as needed
enum class Key : i32 {
    Space      = 32,
    Apostrophe = 39,
    Comma      = 44,
    Minus      = 45,
    Period     = 46,
    Slash      = 47,
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Semicolon  = 59,
    Equal      = 61,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Escape     = 256,
    Enter      = 257,
    Tab        = 258,
    Backspace  = 259,
    Right      = 262,
    Left       = 263,
    Down       = 264,
    Up         = 265,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    LeftShift  = 340,
    LeftCtrl   = 341,
    LeftAlt    = 342,
};

enum class MouseButton : i32 {
    Left   = 0,
    Right  = 1,
    Middle = 2,
};

class Input {
public:
    // call once with the GLFW window to register callbacks
    static void init(GLFWwindow* window);

    // call at the start of each frame to update state
    static void update();

    // keyboard queries
    static bool is_key_pressed(Key key);   // true only on the frame it was pressed
    static bool is_key_held(Key key);      // true while held down
    static bool is_key_released(Key key);  // true only on the frame it was released

    // mouse queries
    static bool is_mouse_pressed(MouseButton button);
    static bool is_mouse_held(MouseButton button);
    static bool is_mouse_released(MouseButton button);

    static Vec2 get_mouse_position();
    static Vec2 get_mouse_delta();
    static float get_scroll_delta();

private:
    static constexpr int MAX_KEYS = 512;
    static constexpr int MAX_BUTTONS = 8;

    // current and previous frame state
    static bool s_keys[MAX_KEYS];
    static bool s_prev_keys[MAX_KEYS];

    static bool s_buttons[MAX_BUTTONS];
    static bool s_prev_buttons[MAX_BUTTONS];

    static Vec2 s_mouse_pos;
    static Vec2 s_prev_mouse_pos;
    static float s_scroll_delta;

    // GLFW callbacks
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void cursor_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};

} // namespace kairo
