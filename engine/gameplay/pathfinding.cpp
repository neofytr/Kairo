#include "gameplay/pathfinding.h"
#include <queue>
#include <cmath>
#include <algorithm>

namespace kairo {

// -- helpers --

static constexpr float SQRT2 = 1.41421356f;

// 8-directional neighbors (cardinal + diagonal)
static constexpr i32 DX[] = { 1, -1, 0, 0, 1, 1, -1, -1 };
static constexpr i32 DY[] = { 0, 0, 1, -1, 1, -1, 1, -1 };
static constexpr float COST[] = { 1, 1, 1, 1, SQRT2, SQRT2, SQRT2, SQRT2 };

// octile distance heuristic
static float heuristic(i32 x0, i32 y0, i32 x1, i32 y1) {
    float dx = static_cast<float>(std::abs(x1 - x0));
    float dy = static_cast<float>(std::abs(y1 - y0));
    return (dx + dy) + (SQRT2 - 2.0f) * std::min(dx, dy);
}

// -- NavGrid --

NavGrid::NavGrid(i32 width, i32 height, float cell_size) {
    init(width, height, cell_size);
}

void NavGrid::init(i32 width, i32 height, float cell_size) {
    m_width = width;
    m_height = height;
    m_cell_size = cell_size;
    m_walkable.assign(static_cast<size_t>(width * height), true);
}

void NavGrid::set_walkable(i32 x, i32 y, bool walkable) {
    if (in_bounds(x, y)) {
        m_walkable[index(x, y)] = walkable;
    }
}

bool NavGrid::is_walkable(i32 x, i32 y) const {
    return in_bounds(x, y) && m_walkable[index(x, y)];
}

Vec2 NavGrid::grid_to_world(i32 x, i32 y) const {
    // return center of cell
    return {
        m_origin.x + (static_cast<float>(x) + 0.5f) * m_cell_size,
        m_origin.y + (static_cast<float>(y) + 0.5f) * m_cell_size
    };
}

void NavGrid::world_to_grid(const Vec2& world, i32& gx, i32& gy) const {
    gx = static_cast<i32>(std::floor((world.x - m_origin.x) / m_cell_size));
    gy = static_cast<i32>(std::floor((world.y - m_origin.y) / m_cell_size));
}

bool NavGrid::in_bounds(i32 x, i32 y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

i32 NavGrid::index(i32 x, i32 y) const {
    return y * m_width + x;
}

std::vector<Vec2> NavGrid::find_path(const Vec2& start, const Vec2& end) const {
    i32 sx, sy, ex, ey;
    world_to_grid(start, sx, sy);
    world_to_grid(end, ex, ey);

    // bail if endpoints are out of bounds or blocked
    if (!is_walkable(sx, sy) || !is_walkable(ex, ey)) {
        return {};
    }

    // trivial case
    if (sx == ex && sy == ey) {
        return { grid_to_world(sx, sy) };
    }

    struct Node {
        i32 x, y;
        float g, f;
    };

    // min-heap by f-cost
    auto cmp = [](const Node& a, const Node& b) { return a.f > b.f; };
    std::priority_queue<Node, std::vector<Node>, decltype(cmp)> open(cmp);

    const i32 total = m_width * m_height;

    // parent encoding: -1 = unvisited
    std::vector<i32> parent(total, -1);
    std::vector<float> g_cost(total, std::numeric_limits<float>::max());
    std::vector<bool> closed(total, false);

    i32 si = index(sx, sy);
    g_cost[si] = 0.0f;
    parent[si] = si; // mark start visited (self-parent)
    open.push({ sx, sy, 0.0f, heuristic(sx, sy, ex, ey) });

    while (!open.empty()) {
        auto cur = open.top();
        open.pop();

        i32 ci = index(cur.x, cur.y);
        if (closed[ci]) continue;
        closed[ci] = true;

        // reached goal
        if (cur.x == ex && cur.y == ey) {
            // reconstruct path
            std::vector<Vec2> path;
            i32 px = ex, py = ey;
            while (!(px == sx && py == sy)) {
                path.push_back(grid_to_world(px, py));
                i32 pi = parent[index(px, py)];
                px = pi % m_width;
                py = pi / m_width;
            }
            path.push_back(grid_to_world(sx, sy));
            std::reverse(path.begin(), path.end());
            return path;
        }

        // expand neighbors
        for (i32 d = 0; d < 8; ++d) {
            i32 nx = cur.x + DX[d];
            i32 ny = cur.y + DY[d];

            if (!is_walkable(nx, ny)) continue;

            // for diagonal moves, ensure both adjacent cardinal cells are walkable
            // to prevent cutting corners through walls
            if (d >= 4) {
                if (!is_walkable(cur.x + DX[d], cur.y) ||
                    !is_walkable(cur.x, cur.y + DY[d])) {
                    continue;
                }
            }

            i32 ni = index(nx, ny);
            if (closed[ni]) continue;

            float ng = cur.g + COST[d] * m_cell_size;
            if (ng < g_cost[ni]) {
                g_cost[ni] = ng;
                parent[ni] = ci;
                open.push({ nx, ny, ng, ng + heuristic(nx, ny, ex, ey) });
            }
        }
    }

    // no path found
    return {};
}

} // namespace kairo
