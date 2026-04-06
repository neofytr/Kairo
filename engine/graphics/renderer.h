#pragma once

#include "graphics/camera.h"
#include "graphics/shader.h"
#include "graphics/texture.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

namespace kairo {

// render layers — controls draw order (lower = drawn first = behind)
enum class RenderLayer : i32 {
    Background = 0,
    Default    = 100,
    Foreground = 200,
    UI         = 300,
};

class Renderer {
public:
    static bool init();
    static void shutdown();

    static void begin(const Camera& camera);
    static void end(); // sorts and flushes all queued draw calls

    // set the active layer for subsequent draw calls
    static void set_layer(RenderLayer layer);
    static void set_layer(i32 layer);

    // colored quads
    static void draw_quad(const Vec2& position, const Vec2& size, const Vec4& color);
    static void draw_quad(const Vec3& position, const Vec2& size, const Vec4& color);
    static void draw_quad(const Vec3& position, const Vec2& size, float rotation, const Vec4& color);

    // textured quads
    static void draw_quad(const Vec3& position, const Vec2& size, const Texture& texture,
                          const Vec4& tint = { 1, 1, 1, 1 });
    static void draw_quad(const Vec3& position, const Vec2& size, float rotation,
                          const Texture& texture, const Vec4& tint = { 1, 1, 1, 1 });

    // textured quad with custom UVs (sprite sheets)
    static void draw_quad(const Vec3& position, const Vec2& size, float rotation,
                          const Texture& texture, const Vec2& uv_min, const Vec2& uv_max,
                          const Vec4& tint = { 1, 1, 1, 1 });

    struct Stats {
        int draw_calls = 0;
        int quad_count  = 0;
    };
    static Stats get_stats();
    static void reset_stats();
};

} // namespace kairo
