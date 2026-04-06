#include "physics/spatial_grid.h"

#include <unordered_set>
#include <algorithm>
#include <cmath>

namespace kairo {

void SpatialGrid::clear() {
    m_cells.clear();
}

void SpatialGrid::insert(Entity entity, const AABB& aabb) {
    // find all cells this AABB overlaps
    CellKey min_cell = to_cell(aabb.min.x, aabb.min.y);
    CellKey max_cell = to_cell(aabb.max.x, aabb.max.y);

    for (i32 y = min_cell.y; y <= max_cell.y; y++) {
        for (i32 x = min_cell.x; x <= max_cell.x; x++) {
            m_cells[{x, y}].push_back(entity);
        }
    }
}

std::vector<std::pair<Entity, Entity>> SpatialGrid::get_potential_pairs() const {
    // use a set to avoid duplicate pairs
    // key: min(id_a, id_b) << 32 | max(id_a, id_b)
    std::unordered_set<u64> seen;
    std::vector<std::pair<Entity, Entity>> pairs;

    for (auto& [cell_key, entities] : m_cells) {
        for (size_t i = 0; i < entities.size(); i++) {
            for (size_t j = i + 1; j < entities.size(); j++) {
                u64 id_a = entities[i].id;
                u64 id_b = entities[j].id;

                // canonical ordering
                u64 lo = std::min(id_a, id_b);
                u64 hi = std::max(id_a, id_b);
                u64 pair_key = (lo << 32) | (hi & 0xFFFFFFFF);

                if (seen.insert(pair_key).second) {
                    pairs.push_back({ entities[i], entities[j] });
                }
            }
        }
    }

    return pairs;
}

} // namespace kairo
