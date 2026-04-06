#include "graphics/window.h"
#include "graphics/renderer.h"
#include "graphics/camera.h"
#include "input/input.h"
#include "core/log.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/math_utils.h"

#include <glad/glad.h>
#include <cmath>

int main() {
    kairo::log::info("starting Kairo engine");

    kairo::Window window;
    if (!window.init({ "Kairo Engine", 1280, 720, true })) {
        return 1;
    }

    kairo::Input::init(window.get_native());

    if (!kairo::Renderer::init()) {
        return 1;
    }

    kairo::Camera camera;
    camera.set_orthographic(1280.0f, 720.0f);

    // player-controlled quad
    kairo::Vec3 player_pos = { 0.0f, 0.0f, 0.0f };
    float player_speed = 300.0f;
    float time = 0.0f;

    while (!window.should_close()) {
        kairo::Input::update();
        window.poll_events();

        float dt = 0.016f; // placeholder until step 1.8
        time += dt;

        // close on escape
        if (kairo::Input::is_key_pressed(kairo::Key::Escape)) {
            break;
        }

        // move player with WASD
        kairo::Vec2 move_dir = { 0.0f, 0.0f };
        if (kairo::Input::is_key_held(kairo::Key::W)) move_dir.y += 1.0f;
        if (kairo::Input::is_key_held(kairo::Key::S)) move_dir.y -= 1.0f;
        if (kairo::Input::is_key_held(kairo::Key::A)) move_dir.x -= 1.0f;
        if (kairo::Input::is_key_held(kairo::Key::D)) move_dir.x += 1.0f;

        // normalize so diagonal isn't faster
        if (move_dir.length_sq() > 0.0f) {
            move_dir = move_dir.normalized();
        }

        player_pos.x += move_dir.x * player_speed * dt;
        player_pos.y += move_dir.y * player_speed * dt;

        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        kairo::Renderer::reset_stats();
        kairo::Renderer::begin(camera);

        // background grid
        int count = 15;
        float spacing = 50.0f;
        float offset = (count - 1) * spacing * 0.5f;

        for (int y = 0; y < count; y++) {
            for (int x = 0; x < count; x++) {
                float px = x * spacing - offset;
                float py = y * spacing - offset;

                float r = (std::sin(time + x * 0.3f) + 1.0f) * 0.5f;
                float g = (std::sin(time + y * 0.3f) + 1.0f) * 0.5f;
                float b = (std::sin(time + (x + y) * 0.2f) + 1.0f) * 0.5f;

                kairo::Renderer::draw_quad(
                    kairo::Vec3(px, py, 0.0f),
                    kairo::Vec2(20.0f, 20.0f),
                    kairo::Vec4(r * 0.3f, g * 0.3f, b * 0.3f, 1.0f)
                );
            }
        }

        // draw player (bright white quad)
        kairo::Renderer::draw_quad(
            player_pos,
            kairo::Vec2(40.0f, 40.0f),
            kairo::Vec4(1.0f, 1.0f, 1.0f, 1.0f)
        );

        kairo::Renderer::end();
        window.swap_buffers();
    }

    kairo::Renderer::shutdown();
    kairo::log::info("shutting down");
    return 0;
}
