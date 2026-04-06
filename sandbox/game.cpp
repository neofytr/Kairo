#include "game.h"
#include "graphics/renderer.h"
#include "input/input.h"
#include "core/log.h"
#include "math/math_utils.h"

#include <cmath>

void Game::on_init() {
    m_camera.set_orthographic(1280.0f, 720.0f);

    // create player entity
    auto player = m_world.create();
    m_world.add_component(player, TransformComponent{ { 0.0f, 0.0f, 0.0f } });
    m_world.add_component(player, SpriteComponent{ { 0.9f, 0.95f, 1.0f, 1.0f }, { 40.0f, 40.0f } });
    m_world.add_component(player, VelocityComponent{});
    m_world.add_component(player, PlayerTag{});

    // spawn a grid of background entities
    int grid = 14;
    float spacing = 50.0f;
    float offset = (grid - 1) * spacing * 0.5f;

    for (int y = 0; y < grid; y++) {
        for (int x = 0; x < grid; x++) {
            auto e = m_world.create();
            float px = x * spacing - offset;
            float py = y * spacing - offset;

            m_world.add_component(e, TransformComponent{ { px, py, 0.0f } });

            // gradient coloring based on grid position
            float r = static_cast<float>(x) / grid;
            float g = 0.15f;
            float b = static_cast<float>(y) / grid;
            m_world.add_component(e, SpriteComponent{
                { r * 0.4f + 0.1f, g, b * 0.4f + 0.1f, 0.9f },
                { 20.0f, 20.0f }
            });

            // gentle drifting motion
            float vx = std::sin(px * 0.02f) * 15.0f;
            float vy = std::cos(py * 0.02f) * 15.0f;
            m_world.add_component(e, VelocityComponent{ { vx, vy, 0.0f } });
        }
    }

    kairo::log::info("game initialized — %zu entities", m_world.entity_count());
}

void Game::on_fixed_update(float dt) {
    // player input → velocity (fixed update for consistent physics)
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

    // movement system — integrate velocity
    m_world.query<TransformComponent, VelocityComponent>(
        [&](kairo::Entity, TransformComponent& t, VelocityComponent& v) {
            t.position += v.velocity * dt;
        }
    );
}

void Game::on_update(float dt) {
    m_time += dt;

    // animate background entities — gentle rotation over time
    m_world.query<TransformComponent, SpriteComponent>(
        [&](kairo::Entity e, TransformComponent& t, SpriteComponent& s) {
            // skip the player (it has a PlayerTag but we check by size as a simple filter)
            if (s.size.x > 30.0f) return;
            t.rotation = std::sin(m_time + t.position.x * 0.01f) * 0.4f;
        }
    );
}

void Game::on_render() {
    kairo::Renderer::begin(m_camera);

    // render all visible entities
    m_world.query<TransformComponent, SpriteComponent>(
        [](kairo::Entity, TransformComponent& t, SpriteComponent& s) {
            kairo::Renderer::draw_quad(t.position, s.size, t.rotation, s.color);
        }
    );

    kairo::Renderer::end();
}
