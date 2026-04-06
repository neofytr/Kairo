#include "editor/editor_panels.h"
#include "ecs/world.h"

#include <imgui.h>
#include <cstdio>

namespace kairo {

// ==================== StatsPanel ====================

void StatsPanel::draw(float fps, float dt, const Renderer::Stats& stats, size_t entity_count) {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(220, 120), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Stats")) {
        ImGui::Text("FPS: %.1f", fps);
        ImGui::Text("Frame: %.2f ms", dt * 1000.0f);
        ImGui::Separator();
        ImGui::Text("Draw calls: %d", stats.draw_calls);
        ImGui::Text("Quads: %d", stats.quad_count);
        ImGui::Text("Entities: %zu", entity_count);
    }
    ImGui::End();
}

// ==================== HierarchyPanel ====================

void HierarchyPanel::draw(World& world) {
    ImGui::SetNextWindowPos(ImVec2(10, 140), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(220, 300), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Hierarchy")) {
        world.for_each_entity([&](Entity entity) {
            char label[64];
            snprintf(label, sizeof(label), "Entity %u (gen %u)", entity.index(), entity.generation());

            bool is_selected = (m_selected == entity);
            if (ImGui::Selectable(label, is_selected)) {
                m_selected = entity;
            }
        });
    }
    ImGui::End();
}

// ==================== InspectorPanel ====================

void InspectorPanel::draw(World& world, Entity entity) {
    ImGui::SetNextWindowPos(ImVec2(1040, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(230, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Inspector")) {
        if (!entity.is_valid() || !world.is_alive(entity)) {
            ImGui::TextDisabled("No entity selected");
        } else {
            ImGui::Text("Entity %u (gen %u)", entity.index(), entity.generation());
            ImGui::Separator();

            // draw each registered component editor
            for (auto& drawer : m_drawers) {
                drawer.func(world, entity);
            }
        }
    }
    ImGui::End();
}

} // namespace kairo
