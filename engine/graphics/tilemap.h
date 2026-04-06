#pragma once

#include "core/types.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "graphics/texture.h"
#include <vector>
#include <string>

namespace kairo {

class World;

struct TilesetConfig {
    i32 tile_width = 16;
    i32 tile_height = 16;
    i32 columns = 0;    // auto-calculated from texture width if 0
    i32 rows = 0;       // auto-calculated from texture height if 0
};

class Tilemap {
public:
    Tilemap() = default;

    // initialize with grid dimensions and world-space tile size
    void init(i32 width, i32 height, float tile_size);

    // set the tileset texture and config (columns/rows auto-computed if 0)
    void set_tileset(const Texture* texture, const TilesetConfig& config);

    // set tile at grid position (tile_id = index into tileset, -1 = empty)
    void set_tile(i32 x, i32 y, i32 tile_id);
    void set_tile(i32 x, i32 y, i32 tile_id, const Vec4& tint);
    i32 get_tile(i32 x, i32 y) const;

    // fill entire map with a tile
    void fill(i32 tile_id);

    // clear all tiles to empty (-1)
    void clear();

    // mark which tile IDs act as solid collision
    void set_solid_tiles(const std::vector<i32>& tile_ids);
    bool is_solid(i32 x, i32 y) const;

    // coordinate conversion
    Vec2 tile_to_world(i32 x, i32 y) const;
    void world_to_tile(const Vec2& world, i32& tx, i32& ty) const;

    // render visible tiles using the tileset texture
    void render(const Vec2& camera_pos, const Vec2& view_size);

    // render without texture -- colored quads for debugging
    void render_colored(const Vec2& camera_pos, const Vec2& view_size);

    i32 get_width() const { return m_width; }
    i32 get_height() const { return m_height; }
    float get_tile_size() const { return m_tile_size; }

private:
    i32 m_width = 0;
    i32 m_height = 0;
    float m_tile_size = 32.0f;
    Vec2 m_origin = {0, 0};  // world position of tile (0,0)

    struct TileData {
        i32 tile_id = -1;  // -1 = empty
        Vec4 tint = {1, 1, 1, 1};
    };
    std::vector<TileData> m_tiles;

    const Texture* m_tileset = nullptr;
    TilesetConfig m_tileset_config;

    std::vector<i32> m_solid_tiles;

    bool in_bounds(i32 x, i32 y) const;
    i32 index(i32 x, i32 y) const;

    // compute UV rectangle for a tile ID from the tileset
    void get_tile_uv(i32 tile_id, Vec2& uv_min, Vec2& uv_max) const;
};

} // namespace kairo
