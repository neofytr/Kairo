#pragma once

#include "core/application.h"
#include "scene/scene.h"
#include "scene/scene_serializer.h"
#include "graphics/texture.h"
#include "graphics/light.h"
#include "graphics/camera_controller.h"
#include "editor/editor_panels.h"
#include "physics/physics_world.h"
#include "physics/collision.h"
#include "physics/rigidbody.h"
#include "particles/particle_emitter.h"
#include "audio/audio.h"
#include "graphics/post_process.h"
#include "graphics/effects/bloom.h"
#include "graphics/effects/vignette.h"
#include "gameplay/health.h"
#include "gameplay/steering.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

// --- game components ---

struct TransformComponent {
    kairo::Vec3 position;
    kairo::Vec3 scale = { 1.0f, 1.0f, 1.0f };
    float rotation = 0.0f;
};

struct SpriteComponent {
    kairo::Vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    kairo::Vec4 base_color = { 1.0f, 1.0f, 1.0f, 1.0f }; // original color for flash effects
    kairo::Vec2 size  = { 32.0f, 32.0f };
    int layer = 100;
};

struct VelocityComponent {
    kairo::Vec3 velocity;
};

struct PlayerTag {};

struct BulletTag {
    float lifetime = 2.0f;
    float damage = 1.0f;
};

// enemy types with different behaviors
enum class EnemyType { Chaser, Tank, Swarmer };

struct EnemyTag {
    EnemyType type = EnemyType::Chaser;
    float contact_damage = 10.0f;
    float flash_timer = 0.0f;    // visual flash when hit
    float wander_angle = 0.0f;   // for steering
};

struct WallTag {};

struct PickupTag {
    enum class Type { Health, SpeedBoost };
    Type type = Type::Health;
    float amount = 25.0f;
    float lifetime = 10.0f;
    float bob_offset = 0.0f;
};

// --- scenes ---

class GameplayScene : public kairo::Scene {
public:
    GameplayScene(kairo::SceneManager& scenes);

    void on_enter() override;
    void on_exit() override;
    void on_fixed_update(float dt) override;
    void on_update(float dt) override;
    void on_render() override;
    void on_pause() override;
    void on_resume() override;

private:
    kairo::SceneManager& m_scenes;
    kairo::PhysicsWorld m_physics;
    kairo::ParticleEmitter m_player_trail;
    kairo::ParticleEmitter m_hit_burst;
    kairo::ParticleEmitter m_muzzle_flash;
    kairo::ParticleEmitter m_death_explosion;
    kairo::ParticleEmitter m_pickup_sparkle;
    kairo::LightSystem m_lights;
    kairo::CameraController m_cam_controller;
    kairo::AudioSystem m_audio;
    kairo::PostProcessStack m_post_process;

    float m_player_speed = 320.0f;
    float m_shoot_cooldown = 0.0f;
    float m_time = 0.0f;
    int m_score = 0;
    int m_kills = 0;
    int m_streak = 0;            // kills without getting hit
    float m_streak_timer = 0.0f; // resets streak if no kill in 3s
    int m_wave = 0;
    float m_wave_timer = 0.0f;
    float m_pickup_timer = 0.0f;
    bool m_game_over = false;
    float m_game_over_timer = 0.0f;

    void setup_particles();
    void spawn_player();
    void spawn_wave();
    void spawn_enemy(EnemyType type, const kairo::Vec2& pos);
    void spawn_walls();
    void spawn_pickup();
    void shoot_bullet(const kairo::Vec2& from, const kairo::Vec2& dir);
    void on_enemy_killed(const kairo::Vec2& pos, EnemyType type);
    void on_player_hit(float damage);
    int get_multiplier() const;
};

class PauseScene : public kairo::Scene {
public:
    PauseScene(kairo::SceneManager& scenes);
    void on_enter() override;
    void on_update(float dt) override;
    void on_render() override;
private:
    kairo::SceneManager& m_scenes;
    float m_time = 0.0f;
};

class GameOverScene : public kairo::Scene {
public:
    GameOverScene(kairo::SceneManager& scenes, int score, int wave, int kills);
    void on_enter() override;
    void on_update(float dt) override;
    void on_render() override;
private:
    kairo::SceneManager& m_scenes;
    int m_final_score;
    int m_final_wave;
    int m_final_kills;
    float m_time = 0.0f;
};

// --- application ---

class Game : public kairo::Application {
public:
    void on_init() override;
    void on_shutdown() override;
    void on_editor_ui(float fps, float dt, const kairo::Renderer::Stats& stats) override;

private:
    kairo::StatsPanel m_stats_panel;
    kairo::HierarchyPanel m_hierarchy_panel;
    kairo::InspectorPanel m_inspector_panel;
    void setup_inspector();
};
