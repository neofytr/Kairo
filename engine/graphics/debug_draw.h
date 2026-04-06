#pragma once

#include "core/types.h"
#include "math/vec2.h"
#include "math/vec4.h"
#include "graphics/camera.h"

namespace kairo {

// immediate-mode debug drawing — lines, rects, circles
// draws on top of everything, cleared each frame
// toggle with DebugDraw::set_enabled()
class DebugDraw {
public:
    static bool init();
    static void shutdown();

    static void set_enabled(bool enabled) { s_enabled = enabled; }
    static bool is_enabled() { return s_enabled; }
    static void toggle() { s_enabled = !s_enabled; }

    // queue draw commands (call during update)
    static void line(const Vec2& a, const Vec2& b, const Vec4& color = {0,1,0,1});
    static void rect(const Vec2& center, const Vec2& size, const Vec4& color = {0,1,0,1});
    static void circle(const Vec2& center, float radius, const Vec4& color = {0,1,0,1}, int segments = 24);
    static void arrow(const Vec2& from, const Vec2& to, const Vec4& color = {0,1,0,1});

    // flush all queued lines to the screen (call after scene render)
    static void render(const Camera& camera);

private:
    struct LineVertex {
        Vec2 position;
        Vec4 color;
    };

    static constexpr u32 MAX_LINES = 16384;
    static constexpr u32 MAX_VERTICES = MAX_LINES * 2;

    static u32 s_vao;
    static u32 s_vbo;
    static class Shader* s_shader;
    static bool s_enabled;

    static LineVertex* s_buffer;
    static u32 s_vertex_count;
};

} // namespace kairo
