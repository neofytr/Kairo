#include "input/input.h"
#include <GLFW/glfw3.h>
#include <cstring>

namespace kairo {

// static member definitions
bool Input::s_keys[MAX_KEYS] = {};
bool Input::s_prev_keys[MAX_KEYS] = {};
bool Input::s_buttons[MAX_BUTTONS] = {};
bool Input::s_prev_buttons[MAX_BUTTONS] = {};
Vec2 Input::s_mouse_pos = {};
Vec2 Input::s_prev_mouse_pos = {};
float Input::s_scroll_delta = 0.0f;

void Input::init(GLFWwindow* window) {
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // initialize mouse position
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    s_mouse_pos = { static_cast<float>(mx), static_cast<float>(my) };
    s_prev_mouse_pos = s_mouse_pos;
}

void Input::update() {
    // snapshot current state as previous for next frame
    std::memcpy(s_prev_keys, s_keys, sizeof(s_keys));
    std::memcpy(s_prev_buttons, s_buttons, sizeof(s_buttons));
    s_prev_mouse_pos = s_mouse_pos;
    s_scroll_delta = 0.0f; // reset scroll each frame
}

bool Input::is_key_pressed(Key key) {
    int k = static_cast<int>(key);
    return k >= 0 && k < MAX_KEYS && s_keys[k] && !s_prev_keys[k];
}

bool Input::is_key_held(Key key) {
    int k = static_cast<int>(key);
    return k >= 0 && k < MAX_KEYS && s_keys[k];
}

bool Input::is_key_released(Key key) {
    int k = static_cast<int>(key);
    return k >= 0 && k < MAX_KEYS && !s_keys[k] && s_prev_keys[k];
}

bool Input::is_mouse_pressed(MouseButton button) {
    int b = static_cast<int>(button);
    return b >= 0 && b < MAX_BUTTONS && s_buttons[b] && !s_prev_buttons[b];
}

bool Input::is_mouse_held(MouseButton button) {
    int b = static_cast<int>(button);
    return b >= 0 && b < MAX_BUTTONS && s_buttons[b];
}

bool Input::is_mouse_released(MouseButton button) {
    int b = static_cast<int>(button);
    return b >= 0 && b < MAX_BUTTONS && !s_buttons[b] && s_prev_buttons[b];
}

Vec2 Input::get_mouse_position() {
    return s_mouse_pos;
}

Vec2 Input::get_mouse_delta() {
    return s_mouse_pos - s_prev_mouse_pos;
}

float Input::get_scroll_delta() {
    return s_scroll_delta;
}

// --- GLFW callbacks ---

void Input::key_callback(GLFWwindow*, int key, int, int action, int) {
    if (key < 0 || key >= MAX_KEYS) return;

    if (action == GLFW_PRESS) {
        s_keys[key] = true;
    } else if (action == GLFW_RELEASE) {
        s_keys[key] = false;
    }
}

void Input::mouse_button_callback(GLFWwindow*, int button, int action, int) {
    if (button < 0 || button >= MAX_BUTTONS) return;

    if (action == GLFW_PRESS) {
        s_buttons[button] = true;
    } else if (action == GLFW_RELEASE) {
        s_buttons[button] = false;
    }
}

void Input::cursor_callback(GLFWwindow*, double xpos, double ypos) {
    s_mouse_pos = { static_cast<float>(xpos), static_cast<float>(ypos) };
}

void Input::scroll_callback(GLFWwindow*, double, double yoffset) {
    s_scroll_delta = static_cast<float>(yoffset);
}

} // namespace kairo
