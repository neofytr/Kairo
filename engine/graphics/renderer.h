#pragma once

#include "graphics/camera.h"
#include "graphics/shader.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

namespace kairo {

// high-level rendering API
// wraps the batch renderer and manages state per frame
class Renderer {
public:
    static bool init();
    static void shutdown();

    static void begin(const Camera& camera);
    static void end();

    // draw a colored quad at position with given size
    static void draw_quad(const Vec2& position, const Vec2& size, const Vec4& color);
    static void draw_quad(const Vec3& position, const Vec2& size, const Vec4& color);

    // draw a rotated quad (rotation in radians around z)
    static void draw_quad(const Vec3& position, const Vec2& size, float rotation, const Vec4& color);

    // stats for debugging
    struct Stats {
        int draw_calls = 0;
        int quad_count  = 0;
    };
    static Stats get_stats();
    static void reset_stats();
};

} // namespace kairo
