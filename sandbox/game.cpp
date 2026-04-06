#include "game.h"
#include "graphics/renderer.h"
#include "input/input.h"
#include "core/log.h"
#include "math/math_utils.h"

#include <imgui.h>
#include <cmath>
#include <memory>

// ==================== GameplayScene ====================

GameplayScene::GameplayScene(kairo::SceneManager& scenes)
    : Scene("Gameplay"), m_scenes(scenes) {}

void GameplayScene::setup_serializer() {
    using json = nlohmann::json;

    m_serializer.register_component<TransformComponent>("transform",
        [](const TransformComponent& t) -> json {
            return { {"x", t.position.x}, {"y", t.position.y}, {"z", t.position.z},
                     {"rotation", t.rotation} };
        },
        [](const json& j) -> TransformComponent {
            return { { j["x"], j["y"], j.value("z", 0.0f) },
                     { 1, 1, 1 }, j.value("rotation", 0.0f) };
        }
    );

    m_serializer.register_component<SpriteComponent>("sprite",
        [](const SpriteComponent& s) -> json {
            return { {"r", s.color.x}, {"g", s.color.y}, {"b", s.color.z}, {"a", s.color.w},
                     {"w", s.size.x}, {"h", s.size.y}, {"layer", s.layer} };
        },
        [](const json& j) -> SpriteComponent {
            return { { j["r"], j["g"], j["b"], j["a"] },
                     { j["w"], j["h"] }, j.value("layer", 100) };
        }
    );

    m_serializer.register_component<VelocityComponent>("velocity",
        [](const VelocityComponent& v) -> json {
            return { {"x", v.velocity.x}, {"y", v.velocity.y}, {"z", v.velocity.z} };
        },
        [](const json& j) -> VelocityComponent {
            return { { j["x"], j["y"], j.value("z", 0.0f) } };
        }
    );
}

void GameplayScene::on_enter() {
    m_camera.set_orthographic(1280.0f, 720.0f);
    setup_serializer();

    m_checkerboard.load("assets/textures/checkerboard.png");

    // create player
    auto player = m_world.create();
    m_world.add_component(player, TransformComponent{ { 0.0f, 0.0f, 0.0f } });
    m_world.add_component(player, SpriteComponent{
        { 1.0f, 1.0f, 1.0f, 1.0f }, { 50.0f, 50.0f },
        static_cast<int>(kairo::RenderLayer::Foreground)
    });
    m_world.add_component(player, VelocityComponent{});
    m_world.add_component(player, PlayerTag{});

    // add physics to player
    kairo::ColliderComponent player_col;
    player_col.half_size = { 25.0f, 25.0f };
    m_world.add_component(player, player_col);

    kairo::RigidBodyComponent player_rb;
    player_rb.set_mass(2.0f);
    player_rb.restitution = 0.5f;
    m_world.add_component(player, player_rb);

    // spawn collidable entities in a grid
    int grid = 10;
    float spacing = 60.0f;
    float offset = (grid - 1) * spacing * 0.5f;

    for (int y = 0; y < grid; y++) {
        for (int x = 0; x < grid; x++) {
            auto e = m_world.create();
            float px = x * spacing - offset;
            float py = y * spacing - offset;

            m_world.add_component(e, TransformComponent{ { px, py, 0.0f } });

            float r = static_cast<float>(x) / grid;
            float g = 0.15f;
            float b = static_cast<float>(y) / grid;
            m_world.add_component(e, SpriteComponent{
                { r * 0.4f + 0.1f, g, b * 0.4f + 0.1f, 0.9f },
                { 24.0f, 24.0f },
                static_cast<int>(kairo::RenderLayer::Background)
            });

            // give entities some initial drift
            float vx = std::sin(px * 0.03f) * 20.0f;
            float vy = std::cos(py * 0.03f) * 20.0f;
            m_world.add_component(e, VelocityComponent{ { vx, vy, 0.0f } });

            // add colliders and rigidbodies to all entities
            kairo::ColliderComponent col;
            col.half_size = { 12.0f, 12.0f };
            m_world.add_component(e, col);

            kairo::RigidBodyComponent rb;
            rb.set_mass(0.5f);
            rb.restitution = 0.6f;
            rb.velocity = { vx, vy };
            m_world.add_component(e, rb);
        }
    }

    kairo::log::info("gameplay scene entered — %zu entities", m_world.entity_count());
}

void GameplayScene::on_fixed_update(float dt) {
    // player input
    m_world.query<VelocityComponent, PlayerTag>(
        [&](kairo::Entity, VelocityComponent& vel, PlayerTag&) {
            kairo::Vec2 dir = { 0.0f, 0.0f };
            if (kairo::Input::is_key_held(kairo::Key::W)) dir.y += 1.0f;
            if (kairo::Input::is_key_held(kairo::Key::S)) dir.y -= 1.0f;
            if (kairo::Input::is_key_held(kairo::Key::A)) dir.x -= 1.0f;
            if (kairo::Input::is_key_held(kairo::Key::D)) dir.x += 1.0f;

            if (dir.length_sq() > 0.0f) dir = dir.normalized();
            vel.velocity.x = dir.x * m_player_speed;
            vel.velocity.y = dir.y * m_player_speed;
        }
    );

    // sync rigidbody velocity with VelocityComponent for player
    m_world.query<VelocityComponent, kairo::RigidBodyComponent, PlayerTag>(
        [&](kairo::Entity, VelocityComponent& vel, kairo::RigidBodyComponent& rb, PlayerTag&) {
            rb.velocity = { vel.velocity.x, vel.velocity.y };
        }
    );

    // collect physics bodies from ECS
    std::vector<kairo::PhysicsBody> bodies;
    m_world.query<TransformComponent, kairo::ColliderComponent>(
        [&](kairo::Entity e, TransformComponent& t, kairo::ColliderComponent& col) {
            kairo::PhysicsBody body;
            body.entity = e;
            body.position = { t.position.x, t.position.y };
            body.collider = col;
            if (m_world.has_component<kairo::RigidBodyComponent>(e)) {
                body.rigidbody = &m_world.get_component<kairo::RigidBodyComponent>(e);
            }
            bodies.push_back(body);
        }
    );

    // run physics
    m_physics.step(bodies, dt);

    // write positions back from physics
    for (auto& body : bodies) {
        if (m_world.is_alive(body.entity)) {
            auto& t = m_world.get_component<TransformComponent>(body.entity);
            t.position.x = body.position.x;
            t.position.y = body.position.y;
        }
    }

    // sync rigidbody velocity back to VelocityComponent
    m_world.query<VelocityComponent, kairo::RigidBodyComponent>(
        [&](kairo::Entity, VelocityComponent& vel, kairo::RigidBodyComponent& rb) {
            vel.velocity.x = rb.velocity.x;
            vel.velocity.y = rb.velocity.y;
        }
    );
}

void GameplayScene::on_update(float dt) {
    m_time += dt;

    // F5 to save scene, F9 to load
    if (kairo::Input::is_key_pressed(kairo::Key::F5)) {
        m_serializer.save(m_world, "assets/scene_save.json");
    }
    if (kairo::Input::is_key_pressed(kairo::Key::F9)) {
        // clear existing entities and reload from file
        // (simplified — a proper implementation would destroy all entities first)
        kairo::log::info("loading scene from file...");
        kairo::World new_world;
        if (m_serializer.load(new_world, "assets/scene_save.json")) {
            m_world = std::move(new_world);
            kairo::log::info("scene loaded successfully");
        }
    }

    // push pause scene on P key
    if (kairo::Input::is_key_pressed(kairo::Key::P)) {
        m_scenes.push(std::make_unique<PauseScene>(m_scenes));
    }

    // animate background entities
    m_world.query<TransformComponent, SpriteComponent>(
        [&](kairo::Entity, TransformComponent& t, SpriteComponent& s) {
            if (s.size.x > 30.0f) return;
            t.rotation = std::sin(m_time + t.position.x * 0.01f) * 0.4f;
        }
    );
}

void GameplayScene::on_render() {
    kairo::Renderer::begin(m_camera);

    // render all entities, setting layer per-entity
    m_world.query<TransformComponent, SpriteComponent>(
        [&](kairo::Entity, TransformComponent& t, SpriteComponent& s) {
            kairo::Renderer::set_layer(s.layer);

            bool is_player = s.size.x > 30.0f;
            if (is_player && m_checkerboard.get_id()) {
                kairo::Renderer::draw_quad(t.position, s.size, t.rotation, m_checkerboard, s.color);
            } else {
                kairo::Renderer::draw_quad(t.position, s.size, t.rotation, s.color);
            }
        }
    );

    kairo::Renderer::end();
}

void GameplayScene::on_pause() {
    kairo::log::info("gameplay paused");
}

void GameplayScene::on_resume() {
    kairo::log::info("gameplay resumed");
}

// ==================== PauseScene ====================

PauseScene::PauseScene(kairo::SceneManager& scenes)
    : Scene("Pause"), m_scenes(scenes) {}

void PauseScene::on_enter() {
    m_camera.set_orthographic(1280.0f, 720.0f);
    kairo::log::info("pause screen active — press P to resume");
}

void PauseScene::on_fixed_update(float) {}

void PauseScene::on_update(float dt) {
    m_time += dt;

    // resume on P key
    if (kairo::Input::is_key_pressed(kairo::Key::P)) {
        m_scenes.pop();
    }
}

void PauseScene::on_render() {
    kairo::Renderer::begin(m_camera);

    // dim overlay
    kairo::Renderer::set_layer(kairo::RenderLayer::UI);
    kairo::Renderer::draw_quad(
        kairo::Vec3(0, 0, 0),
        kairo::Vec2(1280, 720),
        kairo::Vec4(0.0f, 0.0f, 0.0f, 0.6f)
    );

    // pulsing "PAUSED" indicator — just a bright quad for now (text comes later)
    float pulse = (std::sin(m_time * 3.0f) + 1.0f) * 0.5f;
    kairo::Renderer::draw_quad(
        kairo::Vec3(0, 0, 0),
        kairo::Vec2(200, 60),
        kairo::Vec4(0.8f, 0.8f, 1.0f, 0.5f + pulse * 0.5f)
    );

    kairo::Renderer::end();
}

// ==================== Game (Application) ====================

void Game::setup_inspector() {
    // register component editors for the inspector panel
    m_inspector_panel.register_component("Transform",
        [](kairo::World& world, kairo::Entity entity) {
            if (!world.has_component<TransformComponent>(entity)) return;
            auto& t = world.get_component<TransformComponent>(entity);

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Position", &t.position.x, 1.0f);
                ImGui::DragFloat3("Scale", &t.scale.x, 0.1f);
                float deg = t.rotation * 180.0f / 3.14159f;
                if (ImGui::DragFloat("Rotation", &deg, 1.0f)) {
                    t.rotation = deg * 3.14159f / 180.0f;
                }
            }
        }
    );

    m_inspector_panel.register_component("Sprite",
        [](kairo::World& world, kairo::Entity entity) {
            if (!world.has_component<SpriteComponent>(entity)) return;
            auto& s = world.get_component<SpriteComponent>(entity);

            if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit4("Color", &s.color.x);
                ImGui::DragFloat2("Size", &s.size.x, 1.0f);
                ImGui::DragInt("Layer", &s.layer, 1);
            }
        }
    );

    m_inspector_panel.register_component("Velocity",
        [](kairo::World& world, kairo::Entity entity) {
            if (!world.has_component<VelocityComponent>(entity)) return;
            auto& v = world.get_component<VelocityComponent>(entity);

            if (ImGui::CollapsingHeader("Velocity")) {
                ImGui::DragFloat3("Velocity", &v.velocity.x, 1.0f);
            }
        }
    );
}

void Game::on_init() {
    m_use_scenes = true;
    setup_inspector();
    m_scenes.push(std::make_unique<GameplayScene>(m_scenes));
    kairo::log::info("game initialized with scene system (F1 to toggle editor)");
}

void Game::on_shutdown() {
    kairo::log::info("game shutting down");
}

void Game::on_editor_ui(float fps, float dt, const kairo::Renderer::Stats& stats) {
    // get the active scene's world for the panels
    auto* active = m_scenes.get_active();
    if (!active) return;

    auto& world = active->get_world();

    m_stats_panel.draw(fps, dt, stats, world.entity_count());
    m_hierarchy_panel.draw(world);
    m_inspector_panel.draw(world, m_hierarchy_panel.get_selected());
}
