#include "game.h"
#include "graphics/renderer.h"
#include "graphics/debug_draw.h"
#include "input/input.h"
#include "core/log.h"
#include "math/math_utils.h"

#include <imgui.h>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <algorithm>

// ==================== GameplayScene ====================

GameplayScene::GameplayScene(kairo::SceneManager& scenes)
    : Scene("Gameplay"), m_scenes(scenes) {}

void GameplayScene::setup_particles() {
    // player trail — cool blue
    kairo::ParticleEmitterConfig trail;
    trail.max_particles = 600;
    trail.emit_rate = 80.0f;
    trail.lifetime = { 0.15f, 0.5f };
    trail.offset = { {-5, -5}, {5, 5} };
    trail.velocity = { {-10, -10}, {10, 10} };
    trail.start_size = { 2.0f, 4.0f };
    trail.end_size = { 0.0f, 0.5f };
    trail.start_color = { {0.4f, 0.6f, 1.0f, 0.6f}, {0.6f, 0.8f, 1.0f, 0.9f} };
    trail.end_color = { {0.1f, 0.2f, 0.5f, 0.0f}, {0.3f, 0.4f, 0.7f, 0.0f} };
    trail.damping = 4.0f;
    m_player_trail.init(trail);

    // enemy hit — orange sparks
    kairo::ParticleEmitterConfig hit;
    hit.max_particles = 400;
    hit.emit_rate = 0.0f;
    hit.looping = false;
    hit.lifetime = { 0.1f, 0.35f };
    hit.velocity = { {-250, -250}, {250, 250} };
    hit.start_size = { 2.0f, 6.0f };
    hit.end_size = { 0.0f, 0.0f };
    hit.start_color = { {1.0f, 0.7f, 0.2f, 1.0f}, {1.0f, 1.0f, 0.4f, 1.0f} };
    hit.end_color = { {1.0f, 0.1f, 0.0f, 0.0f}, {1.0f, 0.3f, 0.0f, 0.0f} };
    hit.damping = 5.0f;
    m_hit_burst.init(hit);

    // muzzle flash — bright yellow
    kairo::ParticleEmitterConfig muzzle;
    muzzle.max_particles = 120;
    muzzle.emit_rate = 0.0f;
    muzzle.looping = false;
    muzzle.lifetime = { 0.03f, 0.1f };
    muzzle.velocity = { {-60, -60}, {60, 60} };
    muzzle.start_size = { 2.0f, 4.0f };
    muzzle.end_size = { 0.0f, 0.0f };
    muzzle.start_color = { {1.0f, 0.9f, 0.6f, 1.0f}, {1.0f, 1.0f, 0.8f, 1.0f} };
    muzzle.end_color = { {1.0f, 0.5f, 0.0f, 0.0f}, {1.0f, 0.7f, 0.2f, 0.0f} };
    muzzle.damping = 8.0f;
    m_muzzle_flash.init(muzzle);

    // big death explosion — white-hot core
    kairo::ParticleEmitterConfig death;
    death.max_particles = 500;
    death.emit_rate = 0.0f;
    death.looping = false;
    death.lifetime = { 0.2f, 0.7f };
    death.velocity = { {-300, -300}, {300, 300} };
    death.start_size = { 4.0f, 12.0f };
    death.end_size = { 0.0f, 1.0f };
    death.start_color = { {1.0f, 0.9f, 0.7f, 1.0f}, {1.0f, 1.0f, 0.9f, 1.0f} };
    death.end_color = { {0.8f, 0.2f, 0.0f, 0.0f}, {1.0f, 0.4f, 0.1f, 0.0f} };
    death.damping = 3.0f;
    m_death_explosion.init(death);

    // pickup sparkle — green
    kairo::ParticleEmitterConfig sparkle;
    sparkle.max_particles = 100;
    sparkle.emit_rate = 0.0f;
    sparkle.looping = false;
    sparkle.lifetime = { 0.2f, 0.5f };
    sparkle.velocity = { {-80, -80}, {80, 80} };
    sparkle.start_size = { 2.0f, 5.0f };
    sparkle.end_size = { 0.0f, 0.0f };
    sparkle.start_color = { {0.3f, 1.0f, 0.4f, 1.0f}, {0.5f, 1.0f, 0.6f, 1.0f} };
    sparkle.end_color = { {0.1f, 0.5f, 0.2f, 0.0f}, {0.2f, 0.7f, 0.3f, 0.0f} };
    sparkle.damping = 4.0f;
    m_pickup_sparkle.init(sparkle);
}

void GameplayScene::setup_tilemap() {
    // 10x10 grid of alternating dark tiles for the arena floor
    m_tilemap.init(10, 10, 80.0f);
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            bool dark = ((x + y) % 2 == 0);
            float b = dark ? 0.06f : 0.045f;
            m_tilemap.set_tile(x, y, 0, { b, b, b + 0.01f, 1.0f });
        }
    }
}

void GameplayScene::setup_wave_spawner() {
    m_wave_spawner.generate_waves(100, 4, 2,
        { "chaser", "swarmer", "tank" }, 8.0f);

    m_wave_spawner.set_spawn_callback([this](const std::string& type, kairo::i32 index) {
        float arena_edge = 420.0f;
        float angle = static_cast<float>(index * 137 + m_wave * 53) * 0.01f;
        float dist = arena_edge + std::fmod(static_cast<float>(index * 37), 100.0f);
        kairo::Vec2 pos = { std::cos(angle) * dist, std::sin(angle) * dist };

        if (type == "chaser") spawn_enemy(EnemyType::Chaser, pos);
        else if (type == "tank") spawn_enemy(EnemyType::Tank, pos);
        else if (type == "swarmer") spawn_enemy(EnemyType::Swarmer, pos);
    });
}

void GameplayScene::spawn_player() {
    auto player = m_world.create();
    m_world.add_component(player, TransformComponent{ { 0.0f, 0.0f, 0.0f } });
    m_world.add_component(player, SpriteComponent{
        { 0.7f, 0.85f, 1.0f, 1.0f }, { 0.7f, 0.85f, 1.0f, 1.0f },
        { 32.0f, 32.0f },
        static_cast<int>(kairo::RenderLayer::Foreground)
    });
    m_world.add_component(player, VelocityComponent{});
    m_world.add_component(player, PlayerTag{});

    kairo::HealthComponent hp;
    hp.current = 100.0f;
    hp.max = 100.0f;
    m_world.add_component(player, hp);

    kairo::InvincibilityComponent iframes;
    iframes.duration = 1.0f;
    m_world.add_component(player, iframes);

    kairo::ColliderComponent col;
    col.shape = kairo::ColliderComponent::Shape::Circle;
    col.radius = 14.0f;
    col.half_size = { 14.0f, 14.0f };
    col.layer = kairo::CollisionLayer::Player;
    col.mask = kairo::CollisionLayer::Enemy | kairo::CollisionLayer::Wall | kairo::CollisionLayer::Pickup;
    m_world.add_component(player, col);

    kairo::RigidBodyComponent rb;
    rb.set_mass(3.0f);
    rb.restitution = 0.1f;
    m_world.add_component(player, rb);

    // tag the player
    m_tags.tag(m_world, player, "player");

    // spawn orbiting shield as child entity
    spawn_shield(player);
}

void GameplayScene::spawn_shield(kairo::Entity player) {
    auto shield = m_world.create();
    m_world.add_component(shield, TransformComponent{ { 50.0f, 0.0f, 0.0f } });
    m_world.add_component(shield, SpriteComponent{
        { 0.3f, 0.9f, 1.0f, 0.8f }, { 0.3f, 0.9f, 1.0f, 0.8f },
        { 12.0f, 12.0f },
        static_cast<int>(kairo::RenderLayer::Foreground)
    });
    m_world.add_component(shield, ShieldTag{ 0.0f, 50.0f, 3.0f, 2.0f });

    kairo::ColliderComponent col;
    col.shape = kairo::ColliderComponent::Shape::Circle;
    col.radius = 6.0f;
    col.half_size = { 6.0f, 6.0f };
    col.is_trigger = true;
    col.layer = kairo::CollisionLayer::Bullet;
    col.mask = kairo::CollisionLayer::Enemy;
    m_world.add_component(shield, col);

    // attach as child of player via hierarchy system
    m_hierarchy.attach(m_world, shield, player);
}

void GameplayScene::spawn_enemy(EnemyType type, const kairo::Vec2& pos) {
    auto e = m_world.create();
    m_world.add_component(e, TransformComponent{ { pos.x, pos.y, 0.0f } });

    float size, speed, hp, damage;
    kairo::Vec4 color;

    switch (type) {
    case EnemyType::Chaser:
        size = 22.0f; speed = 120.0f; hp = 1.0f; damage = 15.0f;
        color = { 1.0f, 0.3f, 0.25f, 1.0f };
        break;
    case EnemyType::Tank:
        size = 36.0f; speed = 55.0f; hp = 4.0f; damage = 25.0f;
        color = { 0.9f, 0.5f, 0.1f, 1.0f };
        break;
    case EnemyType::Swarmer:
        size = 14.0f; speed = 160.0f; hp = 0.5f; damage = 8.0f;
        color = { 1.0f, 0.6f, 0.8f, 1.0f };
        break;
    default:
        size = 22.0f; speed = 120.0f; hp = 1.0f; damage = 15.0f;
        color = { 1.0f, 0.3f, 0.25f, 1.0f };
        break;
    }

    m_world.add_component(e, SpriteComponent{
        color, color, { size, size },
        static_cast<int>(kairo::RenderLayer::Default)
    });

    m_world.add_component(e, VelocityComponent{});

    EnemyTag tag;
    tag.type = type;
    tag.contact_damage = damage;
    tag.speed = speed;
    m_world.add_component(e, tag);

    kairo::HealthComponent ehp;
    ehp.current = hp;
    ehp.max = hp;
    m_world.add_component(e, ehp);

    kairo::ColliderComponent col;
    col.shape = kairo::ColliderComponent::Shape::Circle;
    col.radius = size * 0.45f;
    col.half_size = { size * 0.45f, size * 0.45f };
    col.layer = kairo::CollisionLayer::Enemy;
    col.mask = kairo::CollisionLayer::Player | kairo::CollisionLayer::Bullet |
               kairo::CollisionLayer::Wall | kairo::CollisionLayer::Enemy;
    m_world.add_component(e, col);

    kairo::RigidBodyComponent rb;
    rb.set_mass(type == EnemyType::Tank ? 4.0f : 1.0f);
    rb.restitution = 0.4f;

    // initial velocity toward center
    kairo::Vec2 dir = kairo::Vec2(-pos.x, -pos.y).normalized();
    rb.velocity = { dir.x * speed * 0.5f, dir.y * speed * 0.5f };
    m_world.add_component(e, rb);

    // tag in tag system
    m_tags.tag(m_world, e, "enemy");
}

void GameplayScene::spawn_boss(const kairo::Vec2& pos) {
    auto e = m_world.create();
    m_world.add_component(e, TransformComponent{ { pos.x, pos.y, 0.0f } });

    float size = 50.0f;
    kairo::Vec4 color = { 1.0f, 0.15f, 0.1f, 1.0f };

    m_world.add_component(e, SpriteComponent{
        color, color, { size, size },
        static_cast<int>(kairo::RenderLayer::Default)
    });
    m_world.add_component(e, VelocityComponent{});

    EnemyTag tag;
    tag.type = EnemyType::Boss;
    tag.contact_damage = 30.0f;
    tag.speed = 80.0f;
    m_world.add_component(e, tag);

    kairo::HealthComponent ehp;
    ehp.current = 10.0f;
    ehp.max = 10.0f;
    m_world.add_component(e, ehp);

    kairo::ColliderComponent col;
    col.shape = kairo::ColliderComponent::Shape::Circle;
    col.radius = size * 0.45f;
    col.half_size = { size * 0.45f, size * 0.45f };
    col.layer = kairo::CollisionLayer::Enemy;
    col.mask = kairo::CollisionLayer::Player | kairo::CollisionLayer::Bullet |
               kairo::CollisionLayer::Wall | kairo::CollisionLayer::Enemy;
    m_world.add_component(e, col);

    kairo::RigidBodyComponent rb;
    rb.set_mass(8.0f);
    rb.restitution = 0.2f;
    kairo::Vec2 dir = kairo::Vec2(-pos.x, -pos.y).normalized();
    rb.velocity = { dir.x * 40.0f, dir.y * 40.0f };
    m_world.add_component(e, rb);

    // tag as both enemy and boss
    m_tags.tag(m_world, e, "enemy");
    m_tags.tag(m_world, e, "boss");
}

void GameplayScene::spawn_wave() {
    m_wave++;
    m_wave_timer = 0.0f;

    // the wave spawner handles most spawning, but we also manually spawn for boss waves
    if (m_wave % 5 == 0) {
        // boss wave — spawn a boss from a random edge
        float angle = static_cast<float>(std::rand() % 360) * kairo::DEG_TO_RAD;
        kairo::Vec2 pos = { std::cos(angle) * 450.0f, std::sin(angle) * 450.0f };
        spawn_boss(pos);
    }

    // also spawn regular enemies — increasing difficulty
    int base = 4 + m_wave * 2;
    float arena_edge = 420.0f;

    for (int i = 0; i < base; i++) {
        float angle = static_cast<float>(i) / base * kairo::TAU;
        float dist = arena_edge + std::fmod(static_cast<float>(i * 137 + m_wave * 53), 100.0f);
        kairo::Vec2 pos = { std::cos(angle) * dist, std::sin(angle) * dist };

        EnemyType type;
        if (m_wave <= 2) {
            type = EnemyType::Chaser;
        } else if (m_wave <= 4) {
            type = (i % 3 == 0) ? EnemyType::Tank : EnemyType::Chaser;
        } else {
            int r = i % 5;
            if (r == 0) type = EnemyType::Tank;
            else if (r < 3) type = EnemyType::Swarmer;
            else type = EnemyType::Chaser;
        }

        spawn_enemy(type, pos);
    }

    kairo::log::info("wave %d — %d enemies", m_wave, base);
}

void GameplayScene::spawn_walls() {
    struct WallDef { float x, y, w, h; };
    float arena = 400.0f;
    float thickness = 16.0f;
    WallDef walls[] = {
        { 0, arena + thickness/2, arena * 2 + thickness * 2, thickness },
        { 0, -(arena + thickness/2), arena * 2 + thickness * 2, thickness },
        { -(arena + thickness/2), 0, thickness, arena * 2 },
        { arena + thickness/2, 0, thickness, arena * 2 },
    };

    for (auto& wd : walls) {
        auto e = m_world.create();
        m_world.add_component(e, TransformComponent{ { wd.x, wd.y, 0.0f } });
        m_world.add_component(e, SpriteComponent{
            { 0.2f, 0.2f, 0.25f, 1.0f }, { 0.2f, 0.2f, 0.25f, 1.0f },
            { wd.w, wd.h },
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

void GameplayScene::spawn_pickup() {
    float x = (static_cast<float>(std::rand() % 600) - 300.0f);
    float y = (static_cast<float>(std::rand() % 600) - 300.0f);

    auto e = m_world.create();
    m_world.add_component(e, TransformComponent{ { x, y, 0.0f } });
    m_world.add_component(e, SpriteComponent{
        { 0.3f, 1.0f, 0.4f, 1.0f }, { 0.3f, 1.0f, 0.4f, 1.0f },
        { 18.0f, 18.0f },
        static_cast<int>(kairo::RenderLayer::Default)
    });

    PickupTag pickup;
    pickup.type = PickupTag::Type::Health;
    pickup.amount = 25.0f;
    pickup.bob_offset = static_cast<float>(std::rand() % 100) / 10.0f;
    m_world.add_component(e, pickup);

    kairo::ColliderComponent col;
    col.shape = kairo::ColliderComponent::Shape::Circle;
    col.radius = 12.0f;
    col.half_size = { 12.0f, 12.0f };
    col.is_trigger = true;
    col.layer = kairo::CollisionLayer::Pickup;
    col.mask = kairo::CollisionLayer::Player;
    m_world.add_component(e, col);

    kairo::RigidBodyComponent rb;
    rb.make_static();
    m_world.add_component(e, rb);
}

void GameplayScene::shoot_bullet(const kairo::Vec2& from, const kairo::Vec2& dir) {
    auto bullet = m_world.create();
    kairo::Vec2 d = dir.length_sq() > 0 ? dir.normalized() : kairo::Vec2(0, 1);
    float speed = 650.0f;

    m_world.add_component(bullet, TransformComponent{ { from.x + d.x * 22, from.y + d.y * 22, 0.0f } });
    m_world.add_component(bullet, SpriteComponent{
        { 1.0f, 0.95f, 0.5f, 1.0f }, { 1.0f, 0.95f, 0.5f, 1.0f },
        { 6.0f, 6.0f },
        static_cast<int>(kairo::RenderLayer::Foreground)
    });
    m_world.add_component(bullet, VelocityComponent{ { d.x * speed, d.y * speed, 0.0f } });
    m_world.add_component(bullet, BulletTag{ 1.5f, 1.0f });

    kairo::ColliderComponent col;
    col.shape = kairo::ColliderComponent::Shape::Circle;
    col.radius = 3.0f;
    col.half_size = { 3.0f, 3.0f };
    col.is_trigger = true;
    col.layer = kairo::CollisionLayer::Bullet;
    col.mask = kairo::CollisionLayer::Enemy | kairo::CollisionLayer::Wall;
    m_world.add_component(bullet, col);

    kairo::RigidBodyComponent rb;
    rb.set_mass(0.1f);
    rb.velocity = { d.x * speed, d.y * speed };
    m_world.add_component(bullet, rb);

    m_muzzle_flash.set_position(from + d * 20.0f);
    m_muzzle_flash.burst(6);
    m_cam_controller.add_trauma(0.08f);
    m_audio.play("assets/sounds/shoot.wav", 0.25f);
}

int GameplayScene::get_multiplier() const {
    if (m_streak >= 20) return 5;
    if (m_streak >= 10) return 4;
    if (m_streak >= 5) return 3;
    if (m_streak >= 3) return 2;
    return 1;
}

bool GameplayScene::check_line_of_sight(const kairo::Vec2& from, const kairo::Vec2& to) {
    // raycast from enemy toward player, check if any wall blocks the path
    kairo::Vec2 diff = to - from;
    float dist = diff.length();
    if (dist < 1.0f) return true;

    kairo::Ray2D ray;
    ray.origin = from;
    ray.direction = diff * (1.0f / dist);

    // collect wall AABBs as ray targets
    std::vector<kairo::RayTarget> targets;
    m_world.query<TransformComponent, kairo::ColliderComponent, WallTag>(
        [&](kairo::Entity e, TransformComponent& t, kairo::ColliderComponent& col, WallTag&) {
            kairo::RayTarget rt;
            rt.aabb = col.get_aabb({ t.position.x, t.position.y });
            rt.entity = e;
            targets.push_back(rt);
        }
    );

    auto hit = kairo::raycast(ray, targets, dist);
    return !hit.has_value(); // no wall hit means clear line of sight
}

void GameplayScene::on_enemy_killed(const kairo::Vec2& pos, EnemyType type) {
    // particles
    m_hit_burst.set_position(pos);
    m_death_explosion.set_position(pos);

    int burst_size = 10;
    if (type == EnemyType::Boss) {
        burst_size = 50;
        m_death_explosion.burst(40);
        m_cam_controller.add_trauma(0.7f);
    } else if (type == EnemyType::Tank) {
        burst_size = 30;
        m_death_explosion.burst(20);
        m_cam_controller.add_trauma(0.4f);
    } else {
        burst_size = 12;
        m_cam_controller.add_trauma(0.2f);
    }
    m_hit_burst.burst(burst_size);

    // scoring
    int base_score = 0;
    switch (type) {
    case EnemyType::Chaser:  base_score = 100; break;
    case EnemyType::Tank:    base_score = 300; break;
    case EnemyType::Swarmer: base_score = 50; break;
    case EnemyType::Boss:    base_score = 1000; break;
    }

    m_streak++;
    m_streak_timer = 0.0f;
    int gained = base_score * get_multiplier();
    m_score += gained;
    m_kills++;

    // tween the displayed score smoothly
    m_tweens.tween(&m_score_display, static_cast<float>(m_score), 0.3f, kairo::ease::out_cubic);

    // flash combo indicator when multiplier is active
    if (get_multiplier() > 1) {
        m_combo_flash = 1.0f;
        m_tweens.tween(&m_combo_flash, 0.0f, 0.5f, kairo::ease::out_quad);
    }

    m_audio.play("assets/sounds/hit.wav", 0.5f);
}

void GameplayScene::on_player_hit(float damage) {
    m_world.query<kairo::HealthComponent, kairo::InvincibilityComponent, PlayerTag>(
        [&](kairo::Entity, kairo::HealthComponent& hp, kairo::InvincibilityComponent& inv, PlayerTag&) {
            if (inv.is_active()) return;

            hp.damage(damage);
            inv.activate();
            m_streak = 0;
            m_cam_controller.add_trauma(0.5f);

            if (!hp.is_alive()) {
                m_game_over = true;
            }
        }
    );
}

void GameplayScene::on_enter() {
    m_camera.set_orthographic(1280.0f, 720.0f);
    m_cam_controller.set_camera(&m_camera);
    m_cam_controller.set_smoothing(10.0f);
    m_cam_controller.set_dead_zone({ 20.0f, 20.0f });
    m_cam_controller.set_shake_intensity(10.0f, 0.03f);

    setup_particles();
    setup_tilemap();
    std::srand(static_cast<unsigned>(m_time * 1000 + 42));

    m_lights.init();
    m_lights.set_ambient({ 0.08f, 0.06f, 0.12f }, 0.15f);

    m_audio.init();
    m_audio.set_master_volume(0.6f);

    m_post_process.init(1280, 720);
    auto bloom = std::make_unique<kairo::BloomEffect>();
    bloom->set_threshold(0.5f);
    bloom->set_intensity(0.35f);
    m_post_process.add_effect(std::move(bloom));
    auto vignette = std::make_unique<kairo::VignetteEffect>();
    vignette->set_intensity(0.6f);
    vignette->set_softness(0.55f);
    m_post_process.add_effect(std::move(vignette));

    spawn_player();
    spawn_walls();
    spawn_wave();

    // use TimerManager to spawn pickups every 10 seconds
    m_timers.every(10.0f, [this]() {
        if (!m_game_over) spawn_pickup();
    });

    setup_wave_spawner();

    m_score = 0;
    m_kills = 0;
    m_streak = 0;
    m_score_display = 0.0f;
    m_game_over = false;

    kairo::log::info("=== KAIRO: SURVIVAL ===");
    kairo::log::info("WASD move | Arrows shoot | P pause | F1 editor | F2 debug");
}

void GameplayScene::on_exit() {
    m_lights.shutdown();
    m_post_process.shutdown();
    m_audio.shutdown();
    m_timers.clear();
    m_tweens.clear();
}

void GameplayScene::on_fixed_update(float dt) {
    if (m_game_over) return;

    KAIRO_PROFILE_SCOPE("Physics");

    // player movement
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

    // enemy AI — use steering behaviors with raycasting for line-of-sight
    kairo::Vec2 player_pos = { 0, 0 };
    m_world.query<TransformComponent, PlayerTag>(
        [&](kairo::Entity, TransformComponent& t, PlayerTag&) {
            player_pos = { t.position.x, t.position.y };
        }
    );

    {
        KAIRO_PROFILE_SCOPE("AI Update");

        m_world.query<TransformComponent, kairo::RigidBodyComponent, EnemyTag>(
            [&](kairo::Entity, TransformComponent& t, kairo::RigidBodyComponent& rb, EnemyTag& enemy) {
                kairo::SteeringAgent agent;
                agent.position = { t.position.x, t.position.y };
                agent.velocity = rb.velocity;

                switch (enemy.type) {
                case EnemyType::Chaser:
                    agent.max_speed = 120.0f; agent.max_force = 300.0f;
                    break;
                case EnemyType::Tank:
                    agent.max_speed = 55.0f; agent.max_force = 150.0f;
                    break;
                case EnemyType::Swarmer:
                    agent.max_speed = 160.0f; agent.max_force = 400.0f;
                    break;
                case EnemyType::Boss:
                    agent.max_speed = 80.0f; agent.max_force = 250.0f;
                    break;
                }

                // check line of sight via raycasting
                bool has_los = check_line_of_sight(agent.position, player_pos);

                kairo::Vec2 force;
                if (has_los) {
                    // seek player directly
                    kairo::Vec2 seek_force = kairo::steering::seek(agent, player_pos);
                    kairo::Vec2 wander_force = kairo::steering::wander(agent, enemy.wander_angle);
                    force = seek_force * 0.85f + wander_force * 0.15f;
                } else {
                    // no line of sight — wander until we can see the player
                    force = kairo::steering::wander(agent, enemy.wander_angle);
                }

                force = kairo::steering::truncate(force, agent.max_force);

                rb.velocity += force * dt * rb.inv_mass;
                rb.velocity = kairo::steering::truncate(rb.velocity, agent.max_speed);
            }
        );
    }

    // physics step
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

    for (auto& body : bodies) {
        if (m_world.is_alive(body.entity)) {
            auto& t = m_world.get_component<TransformComponent>(body.entity);
            t.position.x = body.position.x;
            t.position.y = body.position.y;
        }
    }

    // collision response
    std::vector<kairo::Entity> to_destroy;
    for (auto& col : m_physics.get_collisions()) {
        if (!m_world.is_alive(col.a) || !m_world.is_alive(col.b)) continue;

        bool a_bullet = m_world.has_component<BulletTag>(col.a);
        bool b_bullet = m_world.has_component<BulletTag>(col.b);
        bool a_enemy  = m_world.has_component<EnemyTag>(col.a);
        bool b_enemy  = m_world.has_component<EnemyTag>(col.b);
        bool a_player = m_world.has_component<PlayerTag>(col.a);
        bool b_player = m_world.has_component<PlayerTag>(col.b);
        bool a_pickup = m_world.has_component<PickupTag>(col.a);
        bool b_pickup = m_world.has_component<PickupTag>(col.b);
        bool a_wall   = m_world.has_component<WallTag>(col.a);
        bool b_wall   = m_world.has_component<WallTag>(col.b);
        bool a_shield = m_world.has_component<ShieldTag>(col.a);
        bool b_shield = m_world.has_component<ShieldTag>(col.b);

        // bullet hits enemy
        auto handle_bullet_enemy = [&](kairo::Entity bullet, kairo::Entity enemy) {
            auto& ehp = m_world.get_component<kairo::HealthComponent>(enemy);
            auto& bt = m_world.get_component<BulletTag>(bullet);
            ehp.damage(bt.damage);

            auto& esprite = m_world.get_component<SpriteComponent>(enemy);
            auto& etag = m_world.get_component<EnemyTag>(enemy);
            esprite.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            etag.flash_timer = 0.08f;

            to_destroy.push_back(bullet);

            if (!ehp.is_alive()) {
                auto& et = m_world.get_component<TransformComponent>(enemy);
                on_enemy_killed({ et.position.x, et.position.y }, etag.type);
                to_destroy.push_back(enemy);
            } else {
                auto& et = m_world.get_component<TransformComponent>(enemy);
                m_hit_burst.set_position({ et.position.x, et.position.y });
                m_hit_burst.burst(5);
            }
        };

        if (a_bullet && b_enemy) handle_bullet_enemy(col.a, col.b);
        else if (b_bullet && a_enemy) handle_bullet_enemy(col.b, col.a);

        // bullet hits wall
        if ((a_bullet && b_wall) || (b_bullet && a_wall)) {
            to_destroy.push_back(a_bullet ? col.a : col.b);
        }

        // shield damages enemy on contact
        auto handle_shield_enemy = [&](kairo::Entity shield, kairo::Entity enemy) {
            auto& ehp = m_world.get_component<kairo::HealthComponent>(enemy);
            auto& stag = m_world.get_component<ShieldTag>(shield);
            ehp.damage(stag.damage);

            auto& esprite = m_world.get_component<SpriteComponent>(enemy);
            auto& etag = m_world.get_component<EnemyTag>(enemy);
            esprite.color = { 0.3f, 0.9f, 1.0f, 1.0f }; // cyan flash from shield
            etag.flash_timer = 0.1f;

            if (!ehp.is_alive()) {
                auto& et = m_world.get_component<TransformComponent>(enemy);
                on_enemy_killed({ et.position.x, et.position.y }, etag.type);
                to_destroy.push_back(enemy);
            }
        };

        if (a_shield && b_enemy) handle_shield_enemy(col.a, col.b);
        else if (b_shield && a_enemy) handle_shield_enemy(col.b, col.a);

        // enemy touches player
        if ((a_enemy && b_player) || (b_enemy && a_player)) {
            kairo::Entity enemy_e = a_enemy ? col.a : col.b;
            auto& etag = m_world.get_component<EnemyTag>(enemy_e);
            on_player_hit(etag.contact_damage);
        }

        // player picks up item
        auto handle_pickup = [&](kairo::Entity pickup_e) {
            auto& pk = m_world.get_component<PickupTag>(pickup_e);
            auto& pt = m_world.get_component<TransformComponent>(pickup_e);

            if (pk.type == PickupTag::Type::Health) {
                m_world.query<kairo::HealthComponent, PlayerTag>(
                    [&](kairo::Entity, kairo::HealthComponent& hp, PlayerTag&) {
                        hp.heal(pk.amount);
                    }
                );
            }

            m_pickup_sparkle.set_position({ pt.position.x, pt.position.y });
            m_pickup_sparkle.burst(12);
            to_destroy.push_back(pickup_e);
        };

        if (a_pickup && b_player) handle_pickup(col.a);
        else if (b_pickup && a_player) handle_pickup(col.b);
    }

    for (auto e : to_destroy) {
        if (m_world.is_alive(e)) m_world.destroy(e);
    }
}

void GameplayScene::on_update(float dt) {
    m_time += dt;
    m_timers.update(dt);
    m_tweens.update(dt);

    // game over transition
    if (m_game_over) {
        m_game_over_timer += dt;
        if (m_game_over_timer > 2.0f) {
            m_scenes.switch_to(std::make_unique<GameOverScene>(m_scenes, m_score, m_wave, m_kills));
        }
        return;
    }

    m_shoot_cooldown -= dt;

    // shooting with arrow keys (8-directional)
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
            m_shoot_cooldown = 0.10f;
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

    // wave timer — escalating difficulty every 8 seconds
    m_wave_timer += dt;
    if (m_wave_timer > 8.0f) {
        spawn_wave();
    }

    // update wave spawner
    m_wave_spawner.update(dt);

    // streak decay — resets after 3 seconds without a kill
    m_streak_timer += dt;
    if (m_streak_timer > 3.0f && m_streak > 0) {
        m_streak = 0;
    }

    // expire pickups
    std::vector<kairo::Entity> pickup_expired;
    m_world.query<PickupTag>(
        [&](kairo::Entity e, PickupTag& pk) {
            pk.lifetime -= dt;
            if (pk.lifetime <= 0.0f) pickup_expired.push_back(e);
        }
    );
    for (auto e : pickup_expired) {
        if (m_world.is_alive(e)) m_world.destroy(e);
    }

    // enemy flash decay
    m_world.query<SpriteComponent, EnemyTag>(
        [&](kairo::Entity, SpriteComponent& s, EnemyTag& enemy) {
            if (enemy.flash_timer > 0.0f) {
                enemy.flash_timer -= dt;
                if (enemy.flash_timer <= 0.0f) {
                    s.color = s.base_color;
                }
            }
        }
    );

    // rotate enemies toward player
    kairo::Vec2 player_pos = { 0, 0 };
    m_world.query<TransformComponent, PlayerTag>(
        [&](kairo::Entity, TransformComponent& t, PlayerTag&) {
            player_pos = { t.position.x, t.position.y };
        }
    );
    m_world.query<TransformComponent, EnemyTag>(
        [&](kairo::Entity, TransformComponent& t, EnemyTag&) {
            kairo::Vec2 to_player = { player_pos.x - t.position.x, player_pos.y - t.position.y };
            if (to_player.length_sq() > 1.0f)
                t.rotation = std::atan2(to_player.y, to_player.x);
        }
    );

    // player visual feedback: blink during i-frames, smooth rotation toward movement
    m_world.query<TransformComponent, SpriteComponent, kairo::InvincibilityComponent, VelocityComponent, PlayerTag>(
        [&](kairo::Entity, TransformComponent& t, SpriteComponent& s,
            kairo::InvincibilityComponent& inv, VelocityComponent& vel, PlayerTag&) {
            inv.update(dt);

            if (inv.is_active()) {
                float blink = std::sin(m_time * 20.0f);
                s.color.w = blink > 0.0f ? 1.0f : 0.3f;
            } else {
                s.color.w = 1.0f;
            }

            // smooth rotation toward movement direction
            if (vel.velocity.x * vel.velocity.x + vel.velocity.y * vel.velocity.y > 100.0f) {
                float target_rot = std::atan2(vel.velocity.y, vel.velocity.x);
                float diff = target_rot - t.rotation;
                while (diff > kairo::PI) diff -= kairo::TAU;
                while (diff < -kairo::PI) diff += kairo::TAU;
                t.rotation += diff * dt * 12.0f;
            }
        }
    );

    // update orbiting shield position (entity hierarchy)
    m_world.query<TransformComponent, ShieldTag>(
        [&](kairo::Entity e, TransformComponent& t, ShieldTag& shield) {
            shield.orbit_angle += shield.orbit_speed * dt;
            if (shield.orbit_angle > kairo::TAU) shield.orbit_angle -= kairo::TAU;

            // get parent (player) position via hierarchy
            kairo::Entity parent = m_hierarchy.get_parent(m_world, e);
            if (parent.is_valid() && m_world.is_alive(parent)) {
                auto& pt = m_world.get_component<TransformComponent>(parent);
                t.position.x = pt.position.x + std::cos(shield.orbit_angle) * shield.orbit_radius;
                t.position.y = pt.position.y + std::sin(shield.orbit_angle) * shield.orbit_radius;
            }

            // rotate the shield sprite for visual flair
            t.rotation = shield.orbit_angle;
        }
    );

    // pickup bobbing and spinning
    m_world.query<TransformComponent, PickupTag>(
        [&](kairo::Entity, TransformComponent& t, PickupTag& pk) {
            t.position.y += std::sin(m_time * 3.0f + pk.bob_offset) * 0.3f;
            t.rotation += dt * 2.0f;
        }
    );

    // pause
    if (kairo::Input::is_key_pressed(kairo::Key::P)) {
        m_scenes.push(std::make_unique<PauseScene>(m_scenes));
    }

    // toggle debug draw on F2
    if (kairo::Input::is_key_pressed(kairo::Key::F2)) {
        m_debug_draw = !m_debug_draw;
        kairo::DebugDraw::set_enabled(m_debug_draw);
    }

    // camera + particles
    m_world.query<TransformComponent, PlayerTag>(
        [&](kairo::Entity, TransformComponent& t, PlayerTag&) {
            m_player_trail.set_position({ t.position.x, t.position.y });
            m_cam_controller.set_target({ t.position.x, t.position.y });
            m_audio.set_listener_position({ t.position.x, t.position.y });
        }
    );

    m_cam_controller.update(dt);
    m_player_trail.update(dt);
    m_hit_burst.update(dt);
    m_muzzle_flash.update(dt);
    m_death_explosion.update(dt);
    m_pickup_sparkle.update(dt);
    m_audio.update();
}

void GameplayScene::on_render() {
    KAIRO_PROFILE_SCOPE("Rendering");

    if (m_post_process.has_effects()) {
        m_post_process.begin_capture();
    }

    kairo::Renderer::begin(m_camera);

    // tilemap floor (rendered with colored quads)
    kairo::Renderer::set_layer(kairo::RenderLayer::Background);
    m_tilemap.render_colored(
        { m_camera.get_position().x, m_camera.get_position().y },
        { 1280.0f, 720.0f }
    );

    // entities
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
    m_death_explosion.render(m_camera);
    m_pickup_sparkle.render(m_camera);

    kairo::Renderer::end();

    // 2D lighting
    m_lights.clear_lights();

    // player light — bright blue
    m_world.query<TransformComponent, PlayerTag>(
        [&](kairo::Entity, TransformComponent& t, PlayerTag&) {
            m_lights.add_light({
                { t.position.x, t.position.y },
                { 0.7f, 0.8f, 1.0f }, 1.6f, 380.0f
            });
        }
    );

    // bullet lights — warm yellow points
    m_world.query<TransformComponent, BulletTag>(
        [&](kairo::Entity, TransformComponent& t, BulletTag&) {
            m_lights.add_light({
                { t.position.x, t.position.y },
                { 1.0f, 0.85f, 0.3f }, 0.7f, 70.0f
            });
        }
    );

    // enemy glow — distinct per type
    m_world.query<TransformComponent, EnemyTag>(
        [&](kairo::Entity, TransformComponent& t, EnemyTag& enemy) {
            kairo::Vec3 glow;
            float intensity = 0.3f, radius = 60.0f;
            switch (enemy.type) {
            case EnemyType::Chaser:
                glow = { 1.0f, 0.2f, 0.15f };
                break;
            case EnemyType::Tank:
                glow = { 0.9f, 0.5f, 0.1f }; intensity = 0.5f; radius = 80.0f;
                break;
            case EnemyType::Swarmer:
                glow = { 1.0f, 0.5f, 0.7f }; intensity = 0.2f; radius = 40.0f;
                break;
            case EnemyType::Boss:
                glow = { 1.0f, 0.1f, 0.05f }; intensity = 1.2f; radius = 150.0f;
                break;
            }
            m_lights.add_light({ { t.position.x, t.position.y }, glow, intensity, radius });
        }
    );

    // pickup glow — green
    m_world.query<TransformComponent, PickupTag>(
        [&](kairo::Entity, TransformComponent& t, PickupTag&) {
            m_lights.add_light({ { t.position.x, t.position.y }, { 0.2f, 0.9f, 0.3f }, 0.5f, 60.0f });
        }
    );

    // shield glow — cyan
    m_world.query<TransformComponent, ShieldTag>(
        [&](kairo::Entity, TransformComponent& t, ShieldTag&) {
            m_lights.add_light({ { t.position.x, t.position.y }, { 0.3f, 0.8f, 1.0f }, 0.4f, 45.0f });
        }
    );

    m_lights.render(m_camera);

    // post-processing (bloom + vignette)
    if (m_post_process.has_effects()) {
        m_post_process.end_and_apply();
    }

    // debug draw — collider visualization (after post-processing, before HUD)
    if (m_debug_draw) {
        m_world.query<TransformComponent, kairo::ColliderComponent>(
            [](kairo::Entity, TransformComponent& t, kairo::ColliderComponent& col) {
                kairo::Vec2 pos = { t.position.x + col.offset.x, t.position.y + col.offset.y };
                if (col.shape == kairo::ColliderComponent::Shape::Circle) {
                    kairo::Vec4 c = col.is_trigger ? kairo::Vec4(1,1,0,0.5f) : kairo::Vec4(0,1,0,0.5f);
                    kairo::DebugDraw::circle(pos, col.radius, c, 16);
                } else {
                    kairo::Vec2 size = col.half_size * 2.0f;
                    kairo::Vec4 c = col.is_trigger ? kairo::Vec4(1,1,0,0.5f) : kairo::Vec4(0,1,0,0.5f);
                    kairo::DebugDraw::rect(pos, size, c);
                }
            }
        );
        kairo::DebugDraw::render(m_camera);
    }

    // === HUD (fixed camera, after post-processing) ===
    kairo::Camera hud_camera;
    hud_camera.set_orthographic(1280.0f, 720.0f);
    kairo::Renderer::begin(hud_camera);
    kairo::Renderer::set_layer(kairo::RenderLayer::UI);

    // health bar background
    kairo::Renderer::draw_quad(
        kairo::Vec3(-520, 330, 0), kairo::Vec2(204, 20),
        kairo::Vec4(0.15f, 0.15f, 0.2f, 0.8f)
    );

    // health bar fill with color gradient
    float hp_ratio = 0.0f;
    m_world.query<kairo::HealthComponent, PlayerTag>(
        [&](kairo::Entity, kairo::HealthComponent& hp, PlayerTag&) {
            hp_ratio = hp.ratio();
        }
    );
    kairo::Vec4 hp_color = hp_ratio > 0.5f
        ? kairo::Vec4(0.2f, 0.9f, 0.3f, 1.0f)
        : hp_ratio > 0.25f
            ? kairo::Vec4(0.9f, 0.8f, 0.2f, 1.0f)
            : kairo::Vec4(0.9f, 0.2f, 0.2f, 1.0f);
    float bar_width = 200.0f * hp_ratio;
    kairo::Renderer::draw_quad(
        kairo::Vec3(-520 - 100 + bar_width / 2, 330, 0),
        kairo::Vec2(bar_width, 16),
        hp_color
    );

    // health text
    char hp_buf[32];
    snprintf(hp_buf, sizeof(hp_buf), "HP");
    kairo::Renderer::draw_text(hp_buf, { -635, 326 }, 1.8f, { 0.7f, 0.7f, 0.8f, 0.8f });

    // score (tweened smooth display)
    char score_buf[64];
    snprintf(score_buf, sizeof(score_buf), "SCORE %d", static_cast<int>(m_score_display));
    kairo::Renderer::draw_text(score_buf, { -620, 300 }, 2.5f, { 0.9f, 0.95f, 1.0f, 1.0f });

    // wave text
    char wave_buf[32];
    snprintf(wave_buf, sizeof(wave_buf), "WAVE %d", m_wave);
    kairo::Renderer::draw_text(wave_buf, { -620, 272 }, 2.0f, { 0.5f, 0.6f, 0.8f, 0.8f });

    // combo multiplier (golden when active, with flash animation)
    int mult = get_multiplier();
    if (mult > 1) {
        char combo_buf[32];
        snprintf(combo_buf, sizeof(combo_buf), "x%d COMBO", mult);
        float glow = 0.8f + m_combo_flash * 0.2f;
        kairo::Renderer::draw_text(combo_buf, { -620, 248 }, 2.2f,
            { 1.0f, glow, 0.2f + m_combo_flash * 0.3f, 1.0f });
    }

    // kills counter
    char kills_buf[32];
    snprintf(kills_buf, sizeof(kills_buf), "KILLS %d", m_kills);
    kairo::Renderer::draw_text(kills_buf, { 420, 330 }, 2.0f, { 0.6f, 0.6f, 0.7f, 0.7f });

    // streak counter
    if (m_streak >= 3) {
        char streak_buf[32];
        snprintf(streak_buf, sizeof(streak_buf), "STREAK %d", m_streak);
        kairo::Renderer::draw_text(streak_buf, { 420, 305 }, 1.8f, { 1.0f, 0.7f, 0.2f, 0.9f });
    }

    // controls hint (fades out)
    if (m_time < 6.0f) {
        float alpha = std::max(0.0f, 1.0f - m_time / 6.0f);
        kairo::Renderer::draw_text("WASD move  ARROWS shoot  P pause  F2 debug", { -370, -320 }, 2.0f,
            { 0.5f, 0.5f, 0.6f, alpha });
    }

    // game over flash
    if (m_game_over) {
        float fade = std::min(m_game_over_timer / 1.5f, 1.0f);
        kairo::Renderer::draw_quad(
            kairo::Vec3(0, 0, 0), kairo::Vec2(1280, 720),
            kairo::Vec4(0.1f, 0.0f, 0.0f, fade * 0.6f)
        );
        if (m_game_over_timer > 0.5f) {
            kairo::Renderer::draw_text("GAME OVER", { -90, 20 }, 3.0f,
                { 1.0f, 0.3f, 0.2f, std::min(1.0f, (m_game_over_timer - 0.5f) * 2.0f) });
        }
    }

    kairo::Renderer::end();
}

void GameplayScene::on_pause() {}
void GameplayScene::on_resume() {}

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

    // dim overlay
    kairo::Renderer::draw_quad(
        kairo::Vec3(0, 0, 0), kairo::Vec2(1280, 720),
        kairo::Vec4(0.0f, 0.0f, 0.05f, 0.7f)
    );

    kairo::Renderer::draw_text("PAUSED", { -60, 30 }, 3.0f,
        { 0.8f, 0.85f, 1.0f, 1.0f });

    float pulse = (std::sin(m_time * 2.5f) + 1.0f) * 0.5f;
    kairo::Renderer::draw_text("press P to resume", { -170, -20 }, 2.0f,
        { 0.5f, 0.5f, 0.6f, 0.5f + pulse * 0.5f });

    kairo::Renderer::end();
}

// ==================== GameOverScene ====================

GameOverScene::GameOverScene(kairo::SceneManager& scenes, int score, int wave, int kills)
    : Scene("GameOver"), m_scenes(scenes),
      m_final_score(score), m_final_wave(wave), m_final_kills(kills) {}

void GameOverScene::on_enter() {
    m_camera.set_orthographic(1280.0f, 720.0f);

    // tween the title dropping in from above
    m_title_y = 200.0f;
    m_stats_alpha = 0.0f;
    m_tweens.tween(&m_title_y, 120.0f, 0.8f, kairo::ease::out_elastic);
    m_tweens.tween(&m_stats_alpha, 1.0f, 1.0f, kairo::ease::out_quad);
}

void GameOverScene::on_update(float dt) {
    m_time += dt;
    m_tweens.update(dt);

    if (kairo::Input::is_key_pressed(kairo::Key::R)) {
        m_scenes.switch_to(std::make_unique<GameplayScene>(m_scenes));
    }
}

void GameOverScene::on_render() {
    kairo::Renderer::begin(m_camera);
    kairo::Renderer::set_layer(kairo::RenderLayer::UI);

    // dark background
    kairo::Renderer::draw_quad(
        kairo::Vec3(0, 0, 0), kairo::Vec2(1280, 720),
        kairo::Vec4(0.05f, 0.02f, 0.08f, 1.0f)
    );

    // title with tween animation
    kairo::Renderer::draw_text("GAME OVER", { -90, m_title_y }, 3.5f,
        { 1.0f, 0.3f, 0.2f, 1.0f });

    // stats with fade-in
    float a = m_stats_alpha;
    char buf[64];
    snprintf(buf, sizeof(buf), "SCORE  %d", m_final_score);
    kairo::Renderer::draw_text(buf, { -120, 50 }, 2.5f, { 0.9f, 0.95f, 1.0f, a });

    snprintf(buf, sizeof(buf), "WAVE   %d", m_final_wave);
    kairo::Renderer::draw_text(buf, { -120, 15 }, 2.0f, { 0.6f, 0.7f, 0.9f, a * 0.9f });

    snprintf(buf, sizeof(buf), "KILLS  %d", m_final_kills);
    kairo::Renderer::draw_text(buf, { -120, -15 }, 2.0f, { 0.6f, 0.7f, 0.9f, a * 0.9f });

    // restart prompt
    float pulse = (std::sin(m_time * 2.0f) + 1.0f) * 0.5f;
    kairo::Renderer::draw_text("press R to restart", { -175, -80 }, 2.0f,
        { 0.5f, 0.5f, 0.7f, 0.4f + pulse * 0.6f });

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
                float deg = t.rotation * 180.0f / 3.14159f;
                if (ImGui::DragFloat("Rotation", &deg, 1.0f))
                    t.rotation = deg * 3.14159f / 180.0f;
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
            }
        }
    );
    m_inspector_panel.register_component("Health",
        [](kairo::World& world, kairo::Entity entity) {
            if (!world.has_component<kairo::HealthComponent>(entity)) return;
            auto& hp = world.get_component<kairo::HealthComponent>(entity);
            if (ImGui::CollapsingHeader("Health")) {
                ImGui::ProgressBar(hp.ratio(), ImVec2(-1, 0), "");
                ImGui::DragFloat("Current", &hp.current, 1.0f, 0, hp.max);
                ImGui::DragFloat("Max", &hp.max, 1.0f);
            }
        }
    );
}

void Game::on_init() {
    m_use_scenes = true;
    setup_inspector();
    m_scenes.push(std::make_unique<GameplayScene>(m_scenes));
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
    m_profiler_panel.draw(fps, dt);
}
