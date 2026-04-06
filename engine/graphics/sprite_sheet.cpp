#include "graphics/sprite_sheet.h"
#include "graphics/texture.h"

namespace kairo {

void SpriteSheet::load(const Texture& texture, i32 cell_width, i32 cell_height) {
    m_cols = texture.get_width() / cell_width;
    m_rows = texture.get_height() / cell_height;

    m_cell_u = static_cast<float>(cell_width) / texture.get_width();
    m_cell_v = static_cast<float>(cell_height) / texture.get_height();
}

SpriteRegion SpriteSheet::get_region(i32 col, i32 row) const {
    SpriteRegion region;
    region.uv_min = { col * m_cell_u, row * m_cell_v };
    region.uv_max = { (col + 1) * m_cell_u, (row + 1) * m_cell_v };
    return region;
}

SpriteRegion SpriteSheet::get_region(i32 index) const {
    i32 col = index % m_cols;
    i32 row = index / m_cols;
    return get_region(col, row);
}

} // namespace kairo
