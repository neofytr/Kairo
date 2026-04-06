#include "graphics/ui.h"
#include "graphics/renderer.h"
#include "graphics/font.h"
#include "graphics/texture.h"
#include "input/input.h"
#include "math/vec3.h"

namespace kairo {

Camera UI::s_camera;
float UI::s_screen_w = 1280.0f;
float UI::s_screen_h = 720.0f;
bool UI::s_active = false;

void UI::begin(float screen_width, float screen_height) {
    s_screen_w = screen_width;
    s_screen_h = screen_height;
    s_active = true;

    // ortho camera centered at origin; offset so (0,0) maps to top-left
    // visible X: [0, screen_w], visible Y: [-screen_h, 0]
    // user coord (x, y) -> engine (x, -y, 0)
    s_camera.set_orthographic(s_screen_w, s_screen_h);
    s_camera.set_position(Vec3(s_screen_w / 2.0f, -s_screen_h / 2.0f, 0.0f));

    Renderer::begin(s_camera);
    Renderer::set_layer(RenderLayer::UI);
}

void UI::end() {
    Renderer::end();
    s_active = false;
}

// -- coordinate helpers --

Vec3 UI::to_engine_pos(const Vec2& ui_pos) {
    return Vec3(ui_pos.x, -ui_pos.y, 0.0f);
}

Vec2 UI::to_engine_pos_2d(const Vec2& ui_pos) {
    return Vec2(ui_pos.x, -ui_pos.y);
}

// -- elements --

void UI::label(const Vec2& pos, const std::string& text, float scale, const Vec4& color) {
    Renderer::draw_text(text, to_engine_pos_2d(pos), scale, color);
}

void UI::label_centered(const Vec2& pos, const std::string& text, float scale, const Vec4& color) {
    float text_w = Renderer::get_default_font().measure_width(text, scale);
    Vec2 centered(pos.x - text_w / 2.0f, pos.y);
    Renderer::draw_text(text, to_engine_pos_2d(centered), scale, color);
}

bool UI::button(const Vec2& pos, const Vec2& size, const std::string& text, float text_scale) {
    bool hovered = is_mouse_over(pos, size);

    // background color
    Vec4 bg = hovered ? Vec4{0.3f, 0.3f, 0.45f, 0.95f}
                      : Vec4{0.2f, 0.2f, 0.3f, 0.9f};
    Vec4 text_color = hovered ? Vec4{1.0f, 1.0f, 1.0f, 1.0f}
                              : Vec4{0.85f, 0.85f, 0.85f, 1.0f};

    // draw panel background (quad is centered on position, so offset to center of rect)
    Vec2 center(pos.x + size.x / 2.0f, pos.y + size.y / 2.0f);
    Renderer::draw_quad(to_engine_pos(center), size, bg);

    // draw centered text
    float text_w = Renderer::get_default_font().measure_width(text, text_scale);
    float text_h = Renderer::get_default_font().get_line_height() * text_scale;
    Vec2 text_pos(center.x - text_w / 2.0f, center.y - text_h / 2.0f);
    Renderer::draw_text(text, to_engine_pos_2d(text_pos), text_scale, text_color);

    return hovered && Input::is_mouse_pressed(MouseButton::Left);
}

void UI::panel(const Vec2& pos, const Vec2& size, const Vec4& color) {
    Vec2 center(pos.x + size.x / 2.0f, pos.y + size.y / 2.0f);
    Renderer::draw_quad(to_engine_pos(center), size, color);
}

void UI::progress_bar(const Vec2& pos, const Vec2& size, float ratio,
                      const Vec4& fill_color, const Vec4& bg_color) {
    // clamp ratio
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    // background
    Vec2 bg_center(pos.x + size.x / 2.0f, pos.y + size.y / 2.0f);
    Renderer::draw_quad(to_engine_pos(bg_center), size, bg_color);

    // fill — anchored to the left edge
    if (ratio > 0.0f) {
        float fill_w = size.x * ratio;
        Vec2 fill_center(pos.x + fill_w / 2.0f, pos.y + size.y / 2.0f);
        Renderer::draw_quad(to_engine_pos(fill_center), Vec2(fill_w, size.y), fill_color);
    }
}

void UI::image(const Vec2& pos, const Vec2& size, const Texture& texture, const Vec4& tint) {
    Vec2 center(pos.x + size.x / 2.0f, pos.y + size.y / 2.0f);
    Renderer::draw_quad(to_engine_pos(center), size, texture, tint);
}

bool UI::is_mouse_over(const Vec2& pos, const Vec2& size) {
    Vec2 mouse = get_mouse_ui_pos();
    return mouse.x >= pos.x && mouse.x <= pos.x + size.x &&
           mouse.y >= pos.y && mouse.y <= pos.y + size.y;
}

Vec2 UI::get_mouse_ui_pos() {
    // GLFW already provides mouse position with (0,0) at top-left in screen pixels
    return Input::get_mouse_position();
}

} // namespace kairo
