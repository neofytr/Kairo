#pragma once

#include "core/application.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

// game components — defined here for now, will move to a components header later
struct TransformComponent {
    kairo::Vec3 position;
    kairo::Vec3 scale = { 1.0f, 1.0f, 1.0f };
    float rotation = 0.0f;
};

struct SpriteComponent {
    kairo::Vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    kairo::Vec2 size  = { 32.0f, 32.0f };
};

struct VelocityComponent {
    kairo::Vec3 velocity;
};

struct PlayerTag {};

class Game : public kairo::Application {
public:
    void on_init() override;
    void on_fixed_update(float dt) override;
    void on_update(float dt) override;
    void on_render() override;

private:
    float m_player_speed = 350.0f;
    float m_time = 0.0f;
};
