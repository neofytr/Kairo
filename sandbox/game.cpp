#include "game.h"
#include "graphics/renderer.h"
#include "graphics/debug_draw.h"
#include "input/input.h"
#include "core/log.h"
#include "math/math_utils.h"

#include <imgui.h>
#include <cmath>
#include <memory>

// ==================== GameplayScene ====================

GameplayScene::GameplayScene(kairo::SceneManager& scenes)
    : Scene("Gameplay"), m_scenes(scenes) {}

void GameplayScene::setup_particles() {
    // player trail
    kairo::ParticleEmitterConfig trail;
    trail.max_particles = 500;
    trail.emit_rate = 60.0f;
    trail.lifetime = { 0.2f, 0.6f };
    trail.offset = { {-6, -6}, {6, 6} };
    trail.velocity = { {-15, -15}, {15, 15} };
    trail.start_size = { 2.0f, 5.0f };
    trail.end_size = { 0.0f, 0.5f };
    trail.start_color = { {0.5f, 0.7f, 1.0f, 0.7f}, {0.7f, 0.9f, 1.0f, 1.0f} };
    trail.end_color = { {0.2f, 0.3f, 0.6f, 0.0f}, {0.4f, 0.5f, 0.8f, 0.0f} };
    trail.damping = 3.0f;
    m_player_trail.init(trail);

    // hit explosion
    kairo::ParticleEmitterConfig hit;
    hit.max_particles = 300;
    hit.emit_rate = 0.0f;
    hit.looping = false;
    hit.lifetime = { 0.15f, 0.4f };
    hit.velocity = { {-200, -200}, {200, 200} };
    hit.start_size = { 3.0f, 8.0f };
    hit.end_size = { 0.0f, 0.0f };
    hit.start_color = { {1.0f, 0.7f, 0.2f, 1.0f}, {1.0f, 1.0f, 0.4f, 1.0f} };
    hit.end_color = { {1.0f, 0.1f, 0.0f, 0.0f}, {1.0f, 0.3f, 0.0f, 0.0f} };
    hit.damping = 4.0f;
    m_hit_burst.init(hit);

    // muzzle flash
    kairo::ParticleEmitterConfig muzzle;
    muzzle.max_particles = 100;
    muzzle.emit_rate = 0.0f;
    muzzle.looping = false;
    muzzle.lifetime = { 0.05f, 0.15f };
    muzzle.velocity = { {-80, -80}, {80, 80} };
    muzzle.start_size = { 2.0f, 5.0f };
    muzzle.end_size = { 0.0f, 0.0f };
    muzzle.start_color = { {1.0f, 0.9f, 0.5f, 1.0f}, {1.0f, 1.0f, 0.8f, 1.0f} };
    muzzle.end_color = { {1.0f, 0.5f, 0.0f, 0.0f}, {1.0f, 0.8f, 0.2f, 0.0f} };
    muzzle.damping = 6.0f;
    m_muzzle_flash.init(muzzle);
}

void GameplayScene::setup_serializer() {
    using json = nlohmann::json;

    m_serializer.register_component<TransformComponent>("transform",
        [](const TransformComponent& t) -> json {
            return { {"x", t.position.x}, {"y", t.position.y}, {"z", t.position.z},
                     {"rotation", t.rotation} };
        },
        [](const json& j) -> TransformComponent {
            return { { j["x"], j["y"], j.value("z", 0.0f) }, { 1, 1, 1 }, j.value("rotation", 0.0f) };
        }
    );

    m_serializer.register_component<SpriteComponent>("sprite",
        [](const SpriteComponent& s) -> json {
            return { {"r", s.color.x}, {"g", s.color.y}, {"b", s.color.z}, {"a", s.color.w},
                     {"w", s.size.x}, {"h", s.size.y}, {"layer", s.layer} };
        },
        [](const json& j) -> SpriteComponent {
            return { { j["r"], j["g"], j["b"], j["a"] }, { j["w"], j["h"] }, j.value("layer", 100) };
        }
    );
}

void GameplayScene::spawn_player() {
    auto player = m_world.create();
    m_world.add_component(player, TransformComponent{ { 0.0f, 0.0f, 0.0f } });
    m_world.add_component(player, SpriteComponent{
        { 0.8f, 0.9f, 1.0f, 1.0f }, { 36.0f, 36.0f },
        static_cast<int>(kairo::RenderLayer::Foreground)
    });
    m_world.add_component(player, VelocityComponent{});
    m_world.add_component(player, PlayerTag{});

    kairo::ColliderComponent col;
    col.half_size = { 18.0f, 18.0f };
    col.layer = kairo::CollisionLayer::Player;
    col.mask = kairo::CollisionLayer::Enemy | kairo::CollisionLayer::Wall;
    m_world.add_component(player, col);

    kairo::RigidBodyComponent rb;
    rb.set_mass(3.0f);
    rb.restitution = 0.2f;
    m_world.add_component(player, rb);
}

void GameplayScene::spawn_enemies(int count) {
    for (int i = 0; i < count; i++) {
        auto e = m_world.create();

        // spawn at random positions around the edges
        float angle = static_cast<float>(i) / count * kairo::TAU;
        float dist = 350.0f + std::fmod(static_cast<float>(i * 137), 150.0f);
        float px = std::cos(angle) * dist;
        float py = std::sin(angle) * dist;

        m_world.add_component(e, TransformComponent{ { px, py, 0.0f } });
        m_world.add_component(e, SpriteComponent{
            { 1.0f, 0.3f, 0.3f, 1.0f }, { 28.0f, 28.0f },
            static_cast<int>(kairo::RenderLayer::Default)
        });

        // enemies drift toward origin (toward player)
        float speed = 30.0f + std::fmod(static_cast<float>(i * 73), 40.0f);
        kairo::Vec2 dir = kairo::Vec2(-px, -py).normalized();
        m_world.add_component(e, VelocityComponent{ { dir.x * speed, dir.y * speed, 0.0f } });
        m_world.add_component(e, EnemyTag{ 1.0f });

        kairo::ColliderComponent col;
        col.half_size = { 14.0f, 14.0f };
        col.layer = kairo::CollisionLayer::Enemy;
        col.mask = kairo::CollisionLayer::Player | kairo::CollisionLayer::Bullet |
                   kairo::CollisionLayer::Wall | kairo::CollisionLayer::Enemy;
        m_world.add_component(e, col);

        kairo::RigidBodyComponent rb;
        rb.set_mass(1.0f);
        rb.restitution = 0.5f;
        rb.velocity = { dir.x * speed, dir.y * speed };
        m_world.add_component(e, rb);
    }
}

void GameplayScene::spawn_walls() {
    // arena boundaries — four walls
    struct WallDef { float x, y, w, h; };
    WallDef walls[] = {
        {    0,  380, 800, 20 }, // top
        {    0, -380, 800, 20 }, // bottom
        { -410,    0,  20, 780 }, // left
        {  410,    0,  20, 780 }, // right
    };

    for (auto& wd : walls) {
        auto e = m_world.create();
        m_world.add_component(e, TransformComponent{ { wd.x, wd.y, 0.0f } });
        m_world.add_component(e, SpriteComponent{
            { 0.25f, 0.25f, 0.3f, 1.0f }, { wd.w, wd.h },
            static_cast<int>(kairo::RenderLayer::Background)
        });
        m_world.add_component(e, WallTag{});

        kairo::ColliderComponent col;
        col.half_size = { wd.w / 2.0f, wd.h / 2.0f };
        col.layer = kairo::CollisionLayer::Wall;
        col.mask = kairo::CollisionLayer::All;
        m_world.add_component(e, col);

        kairo::RigidBodyComponent rb;
        rb.make_static();
        m_world.add_component(e, rb);
    }
}

void GameplayScene::shoot_bullet(const kairo::Vec2& from, const kairo::Vec2& dir) {
    auto bullet = m_world.create();

    kairo::Vec2 d = dir.length_sq() > 0 ? dir.normalized() : kairo::Vec2(0, 1);
    float speed = 600.0f;

    m_world.add_component(bullet, TransformComponent{ { from.x + d.x * 25, from.y + d.y * 25, 0.0f } });
    m_world.add_component(bullet, SpriteComponent{
        { 1.0f, 1.0f, 0.5f, 1.0f }, { 8.0f, 8.0f },
        static_cast<int>(kairo::RenderLayer::Foreground)
    });
    m_world.add_component(bullet, VelocityComponent{ { d.x * speed, d.y * speed, 0.0f } });
    m_world.add_component(bullet, BulletTag{ 2.0f });

    kairo::ColliderComponent col;
    col.half_size = { 4.0f, 4.0f };
    col.is_trigger = true;
    col.layer = kairo::CollisionLayer::Bullet;
    col.mask = kairo::CollisionLayer::Enemy | kairo::CollisionLayer::Wall;
    m_world.add_component(bullet, col);

    kairo::RigidBodyComponent rb;
    rb.set_mass(0.1f);
    rb.velocity = { d.x * speed, d.y * speed };
    m_world.add_component(bullet, rb);

    // muzzle flash at spawn point
    m_muzzle_flash.set_position(from);
    m_muzzle_flash.burst(8);

    m_cam_controller.add_trauma(0.1f);
    m_audio.play("assets/sounds/shoot.wav", 0.3f);
}

void GameplayScene::on_enter() {
    m_camera.set_orthographic(1280.0f, 720.0f);
    m_cam_controller.set_camera(&m_camera);
    m_cam_controller.set_smoothing(8.0f);
    m_cam_controller.set_dead_zone({ 30.0f, 30.0f });
    m_cam_controller.set_shake_intensity(12.0f, 0.04f);

    setup_serializer();
    setup_particles();

    m_checkerboard.load("assets/textures/checkerboard.png");

    // lighting
    m_lights.init();
    m_lights.set_ambient({ 0.12f, 0.1f, 0.18f }, 0.2f);

    // scripting
    m_scripting.init();
    m_audio.init();
    m_audio.set_master_volume(0.5f);

    // post-processing
    m_post_process.init(1280, 720);
    auto bloom = std::make_unique<kairo::BloomEffect>();
    bloom->set_threshold(0.6f);
    bloom->set_intensity(0.4f);
    m_post_process.add_effect(std::move(bloom));
    auto vignette = std::make_unique<kairo::VignetteEffect>();
    vignette->set_intensity(0.5f);
    vignette->set_softness(0.6f);
    m_post_process.add_effect(std::move(vignette));
    m_scripting.set_world(&m_world);
    m_scripting.load_file("assets/scripts/test.lua");

    spawn_player();
    spawn_walls();
    spawn_enemies(8);

    kairo::log::info("=== TOP-DOWN SHOOTER ===");
    kairo::log::info("WASD to move, Arrow keys to shoot");
    kairo::log::info("F1 for editor, P to pause, Escape to quit");
    kairo::log::info("========================");
}

void GameplayScene::on_exit() {
    m_lights.shutdown();
    m_post_process.shutdown();
    m_audio.shutdown();
    m_scripting.shutdown();
}

void GameplayScene::on_fixed_update(float dt) {
    // player input
    m_world.query<VelocityComponent, kairo::RigidBodyComponent, PlayerTag>(
        [&](kairo::Entity, VelocityComponent& vel, kairo::RigidBodyComponent& rb, PlayerTag&) {
            kairo::Vec2 dir = { 0.0f, 0.0f };
            if (kairo::Input::is_key_held(kairo::Key::W)) dir.y += 1.0f;
            if (kairo::Input::is_key_held(kairo::Key::S)) dir.y -= 1.0f;
            if (kairo::Input::is_key_held(kairo::Key::A)) dir.x -= 1.0f;
            if (kairo::Input::is_key_held(kairo::Key::D)) dir.x += 1.0f;

            if (dir.length_sq() > 0.0f) dir = dir.normalized();
            rb.velocity = { dir.x * m_player_speed, dir.y * m_player_speed };
            vel.velocity = { dir.x * m_player_speed, dir.y * m_player_speed, 0 };
        }
    );

    // collect and run physics
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

    m_physics.step(bodies, dt);

    // write positions back
    for (auto& body : bodies) {
        if (m_world.is_alive(body.entity)) {
            auto& t = m_world.get_component<TransformComponent>(body.entity);
            t.position.x = body.position.x;
            t.position.y = body.position.y;
        }
    }

    // check bullet-enemy collisions (triggers)
    std::vector<kairo::Entity> to_destroy;
    for (auto& col : m_physics.get_collisions()) {
        // check if one is a bullet and the other is an enemy
        bool a_bullet = m_world.is_alive(col.a) && m_world.has_component<BulletTag>(col.a);
        bool b_bullet = m_world.is_alive(col.b) && m_world.has_component<BulletTag>(col.b);
        bool a_enemy  = m_world.is_alive(col.a) && m_world.has_component<EnemyTag>(col.a);
        bool b_enemy  = m_world.is_alive(col.b) && m_world.has_component<EnemyTag>(col.b);

        if (a_bullet && b_enemy) {
            auto& t = m_world.get_component<TransformComponent>(col.b);
            m_hit_burst.set_position({ t.position.x, t.position.y });
            m_hit_burst.burst(15);
            m_cam_controller.add_trauma(0.3f);
            to_destroy.push_back(col.a);
            to_destroy.push_back(col.b);
            m_score += 100;
            m_audio.play("assets/sounds/hit.wav", 0.5f);
        } else if (b_bullet && a_enemy) {
            auto& t = m_world.get_component<TransformComponent>(col.a);
            m_hit_burst.set_position({ t.position.x, t.position.y });
            m_hit_burst.burst(15);
            m_cam_controller.add_trauma(0.3f);
            to_destroy.push_back(col.a);
            to_destroy.push_back(col.b);
            m_score += 100;
            m_audio.play("assets/sounds/hit.wav", 0.5f);
        }

        // bullets hitting walls
        if ((a_bullet && m_world.is_alive(col.b) && m_world.has_component<WallTag>(col.b)) ||
            (b_bullet && m_world.is_alive(col.a) && m_world.has_component<WallTag>(col.a))) {
            kairo::Entity bullet = a_bullet ? col.a : col.b;
            to_destroy.push_back(bullet);
        }
    }

    for (auto e : to_destroy) {
        if (m_world.is_alive(e)) m_world.destroy(e);
    }
}

void GameplayScene::on_update(float dt) {
    m_time += dt;
    m_shoot_cooldown -= dt;

    // shooting with arrow keys
    if (m_shoot_cooldown <= 0.0f) {
        kairo::Vec2 shoot_dir = { 0, 0 };
        if (kairo::Input::is_key_held(kairo::Key::Up))    shoot_dir.y += 1.0f;
        if (kairo::Input::is_key_held(kairo::Key::Down))  shoot_dir.y -= 1.0f;
        if (kairo::Input::is_key_held(kairo::Key::Left))  shoot_dir.x -= 1.0f;
        if (kairo::Input::is_key_held(kairo::Key::Right)) shoot_dir.x += 1.0f;

        if (shoot_dir.length_sq() > 0.0f) {
            kairo::Vec2 player_pos = { 0, 0 };
            m_world.query<TransformComponent, PlayerTag>(
                [&](kairo::Entity, TransformComponent& t, PlayerTag&) {
                    player_pos = { t.position.x, t.position.y };
                }
            );
            shoot_bullet(player_pos, shoot_dir.normalized());
            m_shoot_cooldown = 0.12f;
        }
    }

    // expire bullets
    std::vector<kairo::Entity> expired;
    m_world.query<BulletTag>(
        [&](kairo::Entity e, BulletTag& b) {
            b.lifetime -= dt;
            if (b.lifetime <= 0.0f) expired.push_back(e);
        }
    );
    for (auto e : expired) {
        if (m_world.is_alive(e)) m_world.destroy(e);
    }

    // enemy waves
    m_wave_timer += dt;
    if (m_wave_timer > 8.0f) {
        m_wave++;
        spawn_enemies(4 + m_wave * 2);
        m_wave_timer = 0.0f;
        kairo::log::info("wave %d!", m_wave);
    }

    // rotate enemies to face player
    kairo::Vec2 player_pos = { 0, 0 };
    m_world.query<TransformComponent, PlayerTag>(
        [&](kairo::Entity, TransformComponent& t, PlayerTag&) {
            player_pos = { t.position.x, t.position.y };
        }
    );
    m_world.query<TransformComponent, EnemyTag>(
        [&](kairo::Entity, TransformComponent& t, EnemyTag&) {
            kairo::Vec2 to_player = { player_pos.x - t.position.x, player_pos.y - t.position.y };
            t.rotation = std::atan2(to_player.y, to_player.x);
        }
    );

    // pause
    if (kairo::Input::is_key_pressed(kairo::Key::P)) {
        m_scenes.push(std::make_unique<PauseScene>(m_scenes));
    }

    // track player for camera and particles
    m_world.query<TransformComponent, PlayerTag>(
        [&](kairo::Entity, TransformComponent& t, PlayerTag&) {
            m_player_trail.set_position({ t.position.x, t.position.y });
            m_cam_controller.set_target({ t.position.x, t.position.y });
        }
    );

    m_cam_controller.update(dt);
    m_player_trail.update(dt);
    m_hit_burst.update(dt);
    m_muzzle_flash.update(dt);

    // update audio listener position to player
    m_world.query<TransformComponent, PlayerTag>(
        [&](kairo::Entity, TransformComponent& t, PlayerTag&) {
            m_audio.set_listener_position({ t.position.x, t.position.y });
        }
    );
    m_audio.update();
    m_shader_watcher.check();
}

void GameplayScene::on_render() {
    // render scene into post-processing framebuffer
    if (m_post_process.has_effects()) {
        m_post_process.begin_capture();
    }

    kairo::Renderer::begin(m_camera);

    // render all entities
    m_world.query<TransformComponent, SpriteComponent>(
        [&](kairo::Entity, TransformComponent& t, SpriteComponent& s) {
            kairo::Renderer::set_layer(s.layer);
            kairo::Renderer::draw_quad(t.position, s.size, t.rotation, s.color);
        }
    );

    // particles
    kairo::Renderer::set_layer(kairo::RenderLayer::Foreground);
    m_player_trail.render(m_camera);
    m_hit_burst.render(m_camera);
    m_muzzle_flash.render(m_camera);

    kairo::Renderer::end();

    // lighting pass
    m_lights.clear_lights();

    // player light
    m_world.query<TransformComponent, PlayerTag>(
        [&](kairo::Entity, TransformComponent& t, PlayerTag&) {
            m_lights.add_light({
                { t.position.x, t.position.y },
                { 0.8f, 0.85f, 1.0f }, 1.5f, 350.0f
            });
        }
    );

    // bullet lights
    m_world.query<TransformComponent, BulletTag>(
        [&](kairo::Entity, TransformComponent& t, BulletTag&) {
            m_lights.add_light({
                { t.position.x, t.position.y },
                { 1.0f, 0.9f, 0.4f }, 0.8f, 80.0f
            });
        }
    );

    // corner accent lights
    m_lights.add_light({ { -350, 300 }, { 1.0f, 0.3f, 0.2f }, 0.6f, 200.0f });
    m_lights.add_light({ {  350, -300 }, { 0.2f, 0.4f, 1.0f }, 0.6f, 200.0f });

    m_lights.render(m_camera);

    // apply post-processing (bloom + vignette) to the scene
    if (m_post_process.has_effects()) {
        m_post_process.end_and_apply();
    }

    // debug draw — colliders, velocity arrows (F2 to toggle)
    m_world.query<TransformComponent, kairo::ColliderComponent>(
        [](kairo::Entity, TransformComponent& t, kairo::ColliderComponent& col) {
            kairo::Vec2 pos = { t.position.x + col.offset.x, t.position.y + col.offset.y };
            kairo::Vec2 size = col.half_size * 2.0f;
            kairo::Vec4 color = col.is_trigger ? kairo::Vec4(1, 1, 0, 0.6f) : kairo::Vec4(0, 1, 0, 0.6f);
            kairo::DebugDraw::rect(pos, size, color);
        }
    );

    // velocity arrows on moving entities
    m_world.query<TransformComponent, kairo::RigidBodyComponent>(
        [](kairo::Entity, TransformComponent& t, kairo::RigidBodyComponent& rb) {
            if (rb.velocity.length_sq() > 1.0f) {
                kairo::Vec2 from = { t.position.x, t.position.y };
                kairo::Vec2 to = from + rb.velocity * 0.3f;
                kairo::DebugDraw::arrow(from, to, { 0.3f, 0.6f, 1.0f, 0.5f });
            }
        }
    );

    kairo::DebugDraw::render(m_camera);

    // === HUD pass — after post-processing so text isn't bloomed/vignetted ===
    kairo::Camera hud_camera;
    hud_camera.set_orthographic(1280.0f, 720.0f);

    kairo::Renderer::begin(hud_camera);
    kairo::Renderer::set_layer(kairo::RenderLayer::UI);

    char score_buf[64];
    snprintf(score_buf, sizeof(score_buf), "SCORE %d", m_score);
    kairo::Renderer::draw_text(score_buf, { -620, 330 }, 2.5f, { 0.9f, 0.95f, 1.0f, 1.0f });

    char wave_buf[32];
    snprintf(wave_buf, sizeof(wave_buf), "WAVE %d", m_wave);
    kairo::Renderer::draw_text(wave_buf, { -620, 300 }, 2.0f, { 0.6f, 0.7f, 0.9f, 0.8f });

    if (m_time < 5.0f) {
        float alpha = std::max(0.0f, 1.0f - m_time / 5.0f);
        kairo::Renderer::draw_text("WASD move  ARROWS shoot", { -230, -320 }, 2.0f,
            { 0.6f, 0.6f, 0.7f, alpha });
    }

    kairo::Renderer::end();
}

void GameplayScene::on_pause() {
    kairo::log::info("game paused");
}

void GameplayScene::on_resume() {
    kairo::log::info("game resumed");
}

// ==================== PauseScene ====================

PauseScene::PauseScene(kairo::SceneManager& scenes)
    : Scene("Pause"), m_scenes(scenes) {}

void PauseScene::on_enter() {
    m_camera.set_orthographic(1280.0f, 720.0f);
}

void PauseScene::on_update(float dt) {
    m_time += dt;
    if (kairo::Input::is_key_pressed(kairo::Key::P)) {
        m_scenes.pop();
    }
}

void PauseScene::on_render() {
    kairo::Renderer::begin(m_camera);

    kairo::Renderer::set_layer(kairo::RenderLayer::UI);
    kairo::Renderer::draw_quad(
        kairo::Vec3(0, 0, 0), kairo::Vec2(1280, 720),
        kairo::Vec4(0.0f, 0.0f, 0.0f, 0.6f)
    );

    float pulse = (std::sin(m_time * 3.0f) + 1.0f) * 0.5f;
    kairo::Renderer::draw_quad(
        kairo::Vec3(0, 0, 0), kairo::Vec2(200, 60),
        kairo::Vec4(0.8f, 0.8f, 1.0f, 0.5f + pulse * 0.5f)
    );

    kairo::Renderer::end();
}

// ==================== Game (Application) ====================

void Game::setup_inspector() {
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
    kairo::log::info("game initialized (F1 for editor)");
}

void Game::on_shutdown() {
    kairo::log::info("game shutting down");
}

void Game::on_editor_ui(float fps, float dt, const kairo::Renderer::Stats& stats) {
    auto* active = m_scenes.get_active();
    if (!active) return;

    auto& world = active->get_world();

    m_stats_panel.draw(fps, dt, stats, world.entity_count());
    m_hierarchy_panel.draw(world);
    m_inspector_panel.draw(world, m_hierarchy_panel.get_selected());
}
