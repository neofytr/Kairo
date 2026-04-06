#pragma once

#include "core/types.h"
#include "math/vec2.h"
#include <vector>

namespace kairo {

class Texture;

// a region within a texture atlas, defined by UV coordinates
struct SpriteRegion {
    Vec2 uv_min = { 0.0f, 0.0f };
    Vec2 uv_max = { 1.0f, 1.0f };
};

// splits a texture into a uniform grid of sprite regions
// typical usage: sprite_sheet.load(texture, 16, 16) for a 16x16 tile grid
class SpriteSheet {
public:
    SpriteSheet() = default;

    // split a texture into a grid of cells
    void load(const Texture& texture, i32 cell_width, i32 cell_height);

    // get a specific cell by grid coordinates (0-based, top-left origin)
    SpriteRegion get_region(i32 col, i32 row) const;

    // get a region by flat index (left-to-right, top-to-bottom)
    SpriteRegion get_region(i32 index) const;

    i32 get_columns() const { return m_cols; }
    i32 get_rows() const { return m_rows; }
    i32 get_total_frames() const { return m_cols * m_rows; }

private:
    i32 m_cols = 0;
    i32 m_rows = 0;
    float m_cell_u = 0.0f; // UV width of one cell
    float m_cell_v = 0.0f; // UV height of one cell
};

// frame-based animation that cycles through sprite regions
struct SpriteAnimation {
    i32 start_frame = 0;
    i32 frame_count = 1;
    float frame_duration = 0.1f; // seconds per frame
    bool looping = true;

    // runtime state
    float elapsed = 0.0f;
    i32 current_frame = 0;

    void update(float dt) {
        elapsed += dt;
        if (elapsed >= frame_duration) {
            elapsed -= frame_duration;
            current_frame++;
            if (current_frame >= frame_count) {
                current_frame = looping ? 0 : frame_count - 1;
            }
        }
    }

    // get the actual sheet index for this animation's current frame
    i32 get_sheet_index() const {
        return start_frame + current_frame;
    }

    void reset() {
        elapsed = 0.0f;
        current_frame = 0;
    }
};

} // namespace kairo
