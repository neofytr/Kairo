#pragma once
#include "core/types.h"
#include "math/vec2.h"
#include <vector>

namespace kairo {

// grid-based navigation map
class NavGrid {
public:
    NavGrid() = default;
    NavGrid(i32 width, i32 height, float cell_size);

    void init(i32 width, i32 height, float cell_size);

    // set whether a cell is walkable
    void set_walkable(i32 x, i32 y, bool walkable);
    bool is_walkable(i32 x, i32 y) const;

    // convert between world and grid coordinates
    Vec2 grid_to_world(i32 x, i32 y) const;
    void world_to_grid(const Vec2& world, i32& gx, i32& gy) const;

    // find a path from start to end (world coordinates)
    // returns a list of world-space waypoints, empty if no path
    std::vector<Vec2> find_path(const Vec2& start, const Vec2& end) const;

    i32 get_width() const { return m_width; }
    i32 get_height() const { return m_height; }
    float get_cell_size() const { return m_cell_size; }

private:
    i32 m_width = 0;
    i32 m_height = 0;
    float m_cell_size = 1.0f;
    Vec2 m_origin = {0, 0}; // world position of grid (0,0)
    std::vector<bool> m_walkable;

    bool in_bounds(i32 x, i32 y) const;
    i32 index(i32 x, i32 y) const;
};

} // namespace kairo
