#pragma once

#include "core/types.h"
#include "ecs/entity.h"
#include "graphics/renderer.h"

namespace kairo {

class World;

// debug stats overlay — FPS, draw calls, entity count
class StatsPanel {
public:
    void draw(float fps, float dt, const Renderer::Stats& stats, size_t entity_count);
};

// entity hierarchy — lists all entities, click to select
class HierarchyPanel {
public:
    void draw(World& world);
    Entity get_selected() const { return m_selected; }

private:
    Entity m_selected;
};

// entity inspector — shows and edits components of the selected entity
// this is generic — concrete component editors are registered via callbacks
class InspectorPanel {
public:
    using DrawComponentFunc = std::function<void(World&, Entity)>;

    // register a component type's editor UI
    void register_component(const std::string& name, DrawComponentFunc func) {
        m_drawers.push_back({ name, std::move(func) });
    }

    void draw(World& world, Entity entity);

private:
    struct ComponentDrawer {
        std::string name;
        DrawComponentFunc func;
    };
    std::vector<ComponentDrawer> m_drawers;
};

} // namespace kairo
