#pragma once

#include "math/vec2.h"
#include "graphics/camera.h"

namespace kairo {

// convert screen pixel coordinates to world coordinates
// screen_pos: (0,0) = top-left, (width,height) = bottom-right
// needs camera position, ortho half-size, and screen dimensions
inline Vec2 screen_to_world(const Vec2& screen_pos, const Vec2& camera_pos,
                            const Vec2& ortho_half_size, i32 screen_width, i32 screen_height) {
    // normalize to [-1, 1]
    float nx = (screen_pos.x / screen_width) * 2.0f - 1.0f;
    float ny = 1.0f - (screen_pos.y / screen_height) * 2.0f;  // flip Y

    // scale by ortho half-size and offset by camera
    return {
        camera_pos.x + nx * ortho_half_size.x,
        camera_pos.y + ny * ortho_half_size.y
    };
}

// world to screen (inverse)
inline Vec2 world_to_screen(const Vec2& world_pos, const Vec2& camera_pos,
                            const Vec2& ortho_half_size, i32 screen_width, i32 screen_height) {
    float nx = (world_pos.x - camera_pos.x) / ortho_half_size.x;
    float ny = (world_pos.y - camera_pos.y) / ortho_half_size.y;

    return {
        (nx + 1.0f) * 0.5f * screen_width,
        (1.0f - ny) * 0.5f * screen_height
    };
}

} // namespace kairo
