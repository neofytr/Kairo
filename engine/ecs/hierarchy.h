#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include "ecs/world.h"
#include "math/vec3.h"

#include <vector>
#include <unordered_map>
#include <algorithm>

namespace kairo {

// component that establishes parent-child relationship
struct ParentComponent {
    Entity parent;
};

struct ChildrenComponent {
    std::vector<Entity> children;
};

// manages the hierarchy and computes world transforms
class HierarchySystem {
public:
    // attach child to parent (adds ParentComponent to child, updates ChildrenComponent on parent)
    void attach(World& world, Entity child, Entity parent) {
        // if child already has a parent, detach first
        if (world.has_component<ParentComponent>(child)) {
            detach(world, child);
        }

        world.add_component<ParentComponent>(child, { parent });

        // ensure parent has a ChildrenComponent
        if (!world.has_component<ChildrenComponent>(parent)) {
            world.add_component<ChildrenComponent>(parent);
        }

        auto& cc = world.get_component<ChildrenComponent>(parent);
        cc.children.push_back(child);
    }

    // detach child from its parent
    void detach(World& world, Entity child) {
        if (!world.has_component<ParentComponent>(child)) return;

        Entity parent = world.get_component<ParentComponent>(child).parent;
        world.remove_component<ParentComponent>(child);

        // remove child from parent's children list
        if (world.is_alive(parent) && world.has_component<ChildrenComponent>(parent)) {
            auto& cc = world.get_component<ChildrenComponent>(parent);
            auto it = std::find(cc.children.begin(), cc.children.end(), child);
            if (it != cc.children.end()) {
                cc.children.erase(it);
            }
        }
    }

    // detach all children from an entity
    void detach_all(World& world, Entity entity) {
        if (!world.has_component<ChildrenComponent>(entity)) return;

        // copy the list since detach modifies it
        auto children = world.get_component<ChildrenComponent>(entity).children;
        for (auto child : children) {
            if (world.is_alive(child) && world.has_component<ParentComponent>(child)) {
                world.remove_component<ParentComponent>(child);
            }
        }

        world.remove_component<ChildrenComponent>(entity);
    }

    // get parent (NULL_ENTITY if none)
    Entity get_parent(World& world, Entity entity) const {
        if (!world.has_component<ParentComponent>(entity)) return NULL_ENTITY;
        return world.get_component<ParentComponent>(entity).parent;
    }

    // get children (empty if none)
    const std::vector<Entity>& get_children(World& world, Entity entity) const {
        static const std::vector<Entity> empty;
        if (!world.has_component<ChildrenComponent>(entity)) return empty;
        return world.get_component<ChildrenComponent>(entity).children;
    }

    // compute world-space position by walking up the parent chain and accumulating local positions
    Vec3 compute_world_position(World& world, Entity entity,
                                const std::unordered_map<u64, Vec3>& local_positions) const {
        Vec3 world_pos;
        Entity current = entity;

        while (current.is_valid() && world.is_alive(current)) {
            auto it = local_positions.find(current.id);
            if (it != local_positions.end()) {
                world_pos += it->second;
            }

            // walk up to parent
            if (world.has_component<ParentComponent>(current)) {
                current = world.get_component<ParentComponent>(current).parent;
            } else {
                break;
            }
        }

        return world_pos;
    }
};

} // namespace kairo
