#include "graphics/tilemap.h"
#include "graphics/renderer.h"
#include <algorithm>
#include <cmath>

namespace kairo {

void Tilemap::init(i32 width, i32 height, float tile_size) {
    m_width = width;
    m_height = height;
    m_tile_size = tile_size;
    m_tiles.resize(static_cast<size_t>(width * height));
    clear();
}

void Tilemap::set_tileset(const Texture* texture, const TilesetConfig& config) {
    m_tileset = texture;
    m_tileset_config = config;

    // auto-compute columns/rows from texture dimensions if not specified
    if (m_tileset && m_tileset_config.columns == 0 && m_tileset_config.tile_width > 0) {
        m_tileset_config.columns = m_tileset->get_width() / m_tileset_config.tile_width;
    }
    if (m_tileset && m_tileset_config.rows == 0 && m_tileset_config.tile_height > 0) {
        m_tileset_config.rows = m_tileset->get_height() / m_tileset_config.tile_height;
    }
}

void Tilemap::set_tile(i32 x, i32 y, i32 tile_id) {
    if (!in_bounds(x, y)) return;
    m_tiles[index(x, y)].tile_id = tile_id;
}

void Tilemap::set_tile(i32 x, i32 y, i32 tile_id, const Vec4& tint) {
    if (!in_bounds(x, y)) return;
    auto& t = m_tiles[index(x, y)];
    t.tile_id = tile_id;
    t.tint = tint;
}

i32 Tilemap::get_tile(i32 x, i32 y) const {
    if (!in_bounds(x, y)) return -1;
    return m_tiles[index(x, y)].tile_id;
}

void Tilemap::fill(i32 tile_id) {
    for (auto& t : m_tiles) {
        t.tile_id = tile_id;
        t.tint = {1, 1, 1, 1};
    }
}

void Tilemap::clear() {
    for (auto& t : m_tiles) {
        t.tile_id = -1;
        t.tint = {1, 1, 1, 1};
    }
}

void Tilemap::set_solid_tiles(const std::vector<i32>& tile_ids) {
    m_solid_tiles = tile_ids;
}

bool Tilemap::is_solid(i32 x, i32 y) const {
    if (!in_bounds(x, y)) return false;
    i32 id = m_tiles[index(x, y)].tile_id;
    if (id < 0) return false;
    return std::find(m_solid_tiles.begin(), m_solid_tiles.end(), id) != m_solid_tiles.end();
}

Vec2 Tilemap::tile_to_world(i32 x, i32 y) const {
    return {
        m_origin.x + static_cast<float>(x) * m_tile_size + m_tile_size * 0.5f,
        m_origin.y + static_cast<float>(y) * m_tile_size + m_tile_size * 0.5f
    };
}

void Tilemap::world_to_tile(const Vec2& world, i32& tx, i32& ty) const {
    tx = static_cast<i32>(std::floor((world.x - m_origin.x) / m_tile_size));
    ty = static_cast<i32>(std::floor((world.y - m_origin.y) / m_tile_size));
}

void Tilemap::render(const Vec2& camera_pos, const Vec2& view_size) {
    if (!m_tileset || m_tileset_config.columns == 0) return;

    // compute visible tile range (frustum cull)
    float half_w = view_size.x * 0.5f;
    float half_h = view_size.y * 0.5f;

    i32 min_x = static_cast<i32>(std::floor((camera_pos.x - half_w - m_origin.x) / m_tile_size));
    i32 min_y = static_cast<i32>(std::floor((camera_pos.y - half_h - m_origin.y) / m_tile_size));
    i32 max_x = static_cast<i32>(std::floor((camera_pos.x + half_w - m_origin.x) / m_tile_size));
    i32 max_y = static_cast<i32>(std::floor((camera_pos.y + half_h - m_origin.y) / m_tile_size));

    // clamp to map bounds
    min_x = std::max(min_x, 0);
    min_y = std::max(min_y, 0);
    max_x = std::min(max_x, m_width - 1);
    max_y = std::min(max_y, m_height - 1);

    Vec2 tile_draw_size{m_tile_size, m_tile_size};

    for (i32 y = min_y; y <= max_y; ++y) {
        for (i32 x = min_x; x <= max_x; ++x) {
            const auto& tile = m_tiles[index(x, y)];
            if (tile.tile_id < 0) continue;

            Vec2 world = tile_to_world(x, y);
            Vec3 pos{world.x, world.y, 0.0f};

            Vec2 uv_min, uv_max;
            get_tile_uv(tile.tile_id, uv_min, uv_max);

            Renderer::draw_quad(pos, tile_draw_size, 0.0f,
                                *m_tileset, uv_min, uv_max, tile.tint);
        }
    }
}

void Tilemap::render_colored(const Vec2& camera_pos, const Vec2& view_size) {
    // compute visible tile range
    float half_w = view_size.x * 0.5f;
    float half_h = view_size.y * 0.5f;

    i32 min_x = static_cast<i32>(std::floor((camera_pos.x - half_w - m_origin.x) / m_tile_size));
    i32 min_y = static_cast<i32>(std::floor((camera_pos.y - half_h - m_origin.y) / m_tile_size));
    i32 max_x = static_cast<i32>(std::floor((camera_pos.x + half_w - m_origin.x) / m_tile_size));
    i32 max_y = static_cast<i32>(std::floor((camera_pos.y + half_h - m_origin.y) / m_tile_size));

    min_x = std::max(min_x, 0);
    min_y = std::max(min_y, 0);
    max_x = std::min(max_x, m_width - 1);
    max_y = std::min(max_y, m_height - 1);

    Vec2 tile_draw_size{m_tile_size, m_tile_size};

    for (i32 y = min_y; y <= max_y; ++y) {
        for (i32 x = min_x; x <= max_x; ++x) {
            const auto& tile = m_tiles[index(x, y)];
            if (tile.tile_id < 0) continue;

            Vec2 world = tile_to_world(x, y);
            Vec3 pos{world.x, world.y, 0.0f};

            // hash tile_id to a deterministic color
            u32 hash = static_cast<u32>(tile.tile_id) * 2654435761u;
            float r = static_cast<float>((hash >> 0) & 0xFF) / 255.0f;
            float g = static_cast<float>((hash >> 8) & 0xFF) / 255.0f;
            float b = static_cast<float>((hash >> 16) & 0xFF) / 255.0f;
            Vec4 color{r, g, b, 1.0f};

            Renderer::draw_quad(pos, tile_draw_size, color);
        }
    }
}

bool Tilemap::in_bounds(i32 x, i32 y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

i32 Tilemap::index(i32 x, i32 y) const {
    return y * m_width + x;
}

void Tilemap::get_tile_uv(i32 tile_id, Vec2& uv_min, Vec2& uv_max) const {
    i32 col = tile_id % m_tileset_config.columns;
    i32 row = tile_id / m_tileset_config.columns;

    float u_cell = static_cast<float>(m_tileset_config.tile_width) / m_tileset->get_width();
    float v_cell = static_cast<float>(m_tileset_config.tile_height) / m_tileset->get_height();

    uv_min = {col * u_cell, row * v_cell};
    uv_max = {(col + 1) * u_cell, (row + 1) * v_cell};
}

} // namespace kairo
