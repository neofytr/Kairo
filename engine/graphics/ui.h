#pragma once

#include "core/types.h"
#include "math/vec2.h"
#include "math/vec4.h"
#include "graphics/camera.h"

#include <string>

namespace kairo {

class Texture;

// Immediate-mode UI system for in-game menus, HUDs, health bars, etc.
// Screen-space coordinates: origin at top-left, Y goes down.
// Call UI::begin() before any UI drawing, UI::end() after.

class UI {
public:
    static void begin(float screen_width = 1280.0f, float screen_height = 720.0f);
    static void end();

    // text
    static void label(const Vec2& pos, const std::string& text, float scale = 2.0f,
                      const Vec4& color = {1, 1, 1, 1});
    static void label_centered(const Vec2& pos, const std::string& text, float scale = 2.0f,
                               const Vec4& color = {1, 1, 1, 1});

    // interactive — returns true on click
    static bool button(const Vec2& pos, const Vec2& size, const std::string& text,
                       float text_scale = 2.0f);

    // decorative
    static void panel(const Vec2& pos, const Vec2& size,
                      const Vec4& color = {0.1f, 0.1f, 0.15f, 0.9f});

    static void progress_bar(const Vec2& pos, const Vec2& size, float ratio,
                             const Vec4& fill_color = {0.2f, 0.8f, 0.3f, 1},
                             const Vec4& bg_color = {0.15f, 0.15f, 0.2f, 0.8f});

    static void image(const Vec2& pos, const Vec2& size, const Texture& texture,
                      const Vec4& tint = {1, 1, 1, 1});

    // utility
    static bool is_mouse_over(const Vec2& pos, const Vec2& size);
    static Vec2 get_mouse_ui_pos();

private:
    // convert top-left UI coords to engine center-origin coords
    static Vec3 to_engine_pos(const Vec2& ui_pos);
    static Vec2 to_engine_pos_2d(const Vec2& ui_pos);

    static Camera s_camera;
    static float s_screen_w;
    static float s_screen_h;
    static bool s_active;
};

} // namespace kairo
