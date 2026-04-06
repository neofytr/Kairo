#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include "ecs/world.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kairo {

// component that stores string tags on an entity
struct TagComponent {
    std::unordered_set<std::string> tags;

    void add(const std::string& tag) { tags.insert(tag); }
    void remove(const std::string& tag) { tags.erase(tag); }
    bool has(const std::string& tag) const { return tags.count(tag) > 0; }
};

// system for querying entities by tag name
class TagSystem {
public:
    // tag an entity (adds TagComponent if needed)
    void tag(World& world, Entity entity, const std::string& tag_name) {
        if (!world.has_component<TagComponent>(entity)) {
            world.add_component<TagComponent>(entity);
        }
        world.get_component<TagComponent>(entity).add(tag_name);
    }

    // remove a tag from an entity
    void untag(World& world, Entity entity, const std::string& tag_name) {
        if (world.has_component<TagComponent>(entity)) {
            world.get_component<TagComponent>(entity).remove(tag_name);
        }
    }

    // check if entity has a tag
    bool has_tag(World& world, Entity entity, const std::string& tag_name) const {
        if (!world.has_component<TagComponent>(entity)) return false;
        return world.get_component<TagComponent>(entity).has(tag_name);
    }

    // find all entities with a given tag
    std::vector<Entity> find_by_tag(World& world, const std::string& tag_name) const {
        std::vector<Entity> result;
        world.for_each_entity([&](Entity e) {
            if (world.has_component<TagComponent>(e)) {
                if (world.get_component<TagComponent>(e).has(tag_name)) {
                    result.push_back(e);
                }
            }
        });
        return result;
    }

    // find first entity with tag (NULL_ENTITY if none)
    Entity find_first(World& world, const std::string& tag_name) const {
        Entity found = NULL_ENTITY;
        world.for_each_entity([&](Entity e) {
            if (found != NULL_ENTITY) return; // already found one
            if (world.has_component<TagComponent>(e)) {
                if (world.get_component<TagComponent>(e).has(tag_name)) {
                    found = e;
                }
            }
        });
        return found;
    }
};

} // namespace kairo
