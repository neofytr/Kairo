#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include "physics/aabb.h"
#include "math/vec2.h"

#include <vector>
#include <unordered_map>

namespace kairo {

// uniform grid for broadphase collision detection
// divides the world into fixed-size cells and only checks
// entities that share the same cell
class SpatialGrid {
public:
    SpatialGrid(float cell_size = 100.0f) : m_cell_size(cell_size) {}

    // clear all cells for the new frame
    void clear();

    // insert an entity with its AABB — may span multiple cells
    void insert(Entity entity, const AABB& aabb);

    // get all unique pairs of entities that share at least one cell
    // this is the broadphase output — narrowphase tests these pairs
    std::vector<std::pair<Entity, Entity>> get_potential_pairs() const;

    void set_cell_size(float size) { m_cell_size = size; }
    float get_cell_size() const { return m_cell_size; }

private:
    float m_cell_size;

    // cell key from grid coordinates
    struct CellKey {
        i32 x, y;
        bool operator==(const CellKey& other) const { return x == other.x && y == other.y; }
    };

    struct CellKeyHash {
        size_t operator()(const CellKey& k) const {
            // simple hash combine
            size_t h = std::hash<i32>()(k.x);
            h ^= std::hash<i32>()(k.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    std::unordered_map<CellKey, std::vector<Entity>, CellKeyHash> m_cells;

    CellKey to_cell(float x, float y) const {
        return {
            static_cast<i32>(std::floor(x / m_cell_size)),
            static_cast<i32>(std::floor(y / m_cell_size))
        };
    }
};

} // namespace kairo
