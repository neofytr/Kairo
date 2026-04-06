#include "game.h"
#include "graphics/renderer.h"
#include "graphics/debug_draw.h"
#include "input/input.h"
#include "core/log.h"
#include "math/math_utils.h"
#include "editor/profiler_panel.h"

#include <imgui.h>
#include <cmath>
#include <algorithm>
#include <memory>

// ==================== DungeonScene ====================

DungeonScene::DungeonScene(kairo::SceneManager& scenes, int floor)
    : Scene("Dungeon"), m_scenes(scenes), m_floor(floor),
      m_rng(static_cast<unsigned>(floor * 7919 + 42)) {}

void DungeonScene::generate_dungeon() {
    m_grid.assign(m_map_h, std::vector<int>(m_map_w, 0));
    m_rooms.clear();

    // BSP-ish room placement
    int max_rooms = 8 + m_floor * 2;
    int attempts = 0;

    while (static_cast<int>(m_rooms.size()) < max_rooms && attempts < 200) {
        attempts++;
        int w = 4 + m_rng() % 5;
        int h = 3 + m_rng() % 4;
        int x = 1 + m_rng() % (m_map_w - w - 2);
        int y = 1 + m_rng() % (m_map_h - h - 2);

        Room room = { x, y, w, h };
        bool overlap = false;
        for (auto& r : m_rooms) {
            if (room.intersects(r, 2)) { overlap = true; break; }
        }
        if (overlap) continue;

        m_rooms.push_back(room);
        carve_room(room);
    }

    // connect rooms with corridors
    for (size_t i = 1; i < m_rooms.size(); i++) {
        carve_corridor(m_rooms[i - 1].center_x(), m_rooms[i - 1].center_y(),
                       m_rooms[i].center_x(), m_rooms[i].center_y());
    }

    // set up tilemap
    m_tilemap.init(m_map_w, m_map_h, m_tile_size);
    for (int y = 0; y < m_map_h; y++) {
        for (int x = 0; x < m_map_w; x++) {
            if (m_grid[y][x] > 0) {
                // floor tiles: alternate slightly for visual variety
                int tile_id = ((x + y) % 2 == 0) ? 1 : 2;
                m_tilemap.set_tile(x, y, tile_id);
            } else {
                m_tilemap.set_tile(x, y, 0); // wall
            }
        }
    }

    // set up navgrid for pathfinding
    m_navgrid.init(m_map_w, m_map_h, m_tile_size);
    for (int y = 0; y < m_map_h; y++) {
        for (int x = 0; x < m_map_w; x++) {
            m_navgrid.set_walkable(x, y, m_grid[y][x] > 0);
        }
    }
}

void DungeonScene::carve_room(const Room& room) {
    for (int y = room.y; y < room.y + room.h; y++) {
        for (int x = room.x; x < room.x + room.w; x++) {
            m_grid[y][x] = 1;
        }
    }
}

void DungeonScene::carve_corridor(int x1, int y1, int x2, int y2) {
    // L-shaped corridor
    int cx = x1;
    while (cx != x2) {
        if (cx >= 0 && cx < m_map_w && y1 >= 0 && y1 < m_map_h)
            m_grid[y1][cx] = 2;
        cx += (x2 > x1) ? 1 : -1;
    }
    int cy = y1;
    while (cy != y2) {
        if (x2 >= 0 && x2 < m_map_w && cy >= 0 && cy < m_map_h)
            m_grid[cy][x2] = 2;
        cy += (y2 > y1) ? 1 : -1;
    }
}

void DungeonScene::populate_dungeon() {
    if (m_rooms.empty()) return;

    // player in first room
    spawn_player(m_rooms[0].center_x(), m_rooms[0].center_y());

    // exit in last room
    auto& last = m_rooms.back();
    spawn_item(last.center_x(), last.center_y(), ItemComp::Exit);

    // enemies in middle rooms
    for (size_t i = 1; i < m_rooms.size(); i++) {
        auto& r = m_rooms[i];
        int enemy_count = 1 + m_floor / 3;
        for (int e = 0; e < enemy_count && e < 3; e++) {
            int ex = r.x + 1 + m_rng() % std::max(1, r.w - 2);
            int ey = r.y + 1 + m_rng() % std::max(1, r.h - 2);
            if (is_walkable(ex, ey)) spawn_enemy(ex, ey);
        }
    }

    // scatter items
    for (size_t i = 1; i < m_rooms.size() - 1; i++) {
        auto& r = m_rooms[i];
        int ix = r.x + m_rng() % r.w;
        int iy = r.y + m_rng() % r.h;
        if (!is_walkable(ix, iy)) continue;

        int roll = m_rng() % 100;
        if (roll < 20) {
            spawn_item(ix, iy, ItemComp::Key);
        } else if (roll < 45) {
            spawn_item(ix, iy, ItemComp::Potion);
        } else if (roll < 65) {
            spawn_item(ix, iy, ItemComp::Treasure);
        }

        // traps in corridors
        if (i > 1 && m_rng() % 100 < 30) {
            // find a corridor tile near this room
            int tx = r.center_x() + (m_rng() % 3 - 1);
            int ty = r.center_y() + (m_rng() % 3 - 1);
            if (is_walkable(tx, ty)) spawn_trap(tx, ty);
        }
    }
}

void DungeonScene::spawn_player(int tx, int ty) {
    auto e = m_world.create();
    auto center = tile_center(tx, ty);
    m_world.add_component(e, Position{ { center.x, center.y, 0 } });
    m_world.add_component(e, Sprite{
        { 0.5f, 0.7f, 1.0f, 1.0f }, { 0.5f, 0.7f, 1.0f, 1.0f },
        { 26.0f, 26.0f }, static_cast<int>(kairo::RenderLayer::Foreground)
    });

    PlayerComp pc;
    pc.tile_x = tx; pc.tile_y = ty;
    m_world.add_component(e, pc);

    kairo::HealthComponent hp;
    hp.current = 100; hp.max = 100;
    m_world.add_component(e, hp);

    kairo::InvincibilityComponent inv;
    inv.duration = 0.5f;
    m_world.add_component(e, inv);

    m_tags.tag(m_world, e, "player");
}

void DungeonScene::spawn_enemy(int tx, int ty) {
    auto e = m_world.create();
    auto center = tile_center(tx, ty);

    // vary enemy types by floor
    bool is_strong = m_rng() % 100 < (m_floor * 10);
    float sz = is_strong ? 28.0f : 22.0f;
    kairo::Vec4 color = is_strong
        ? kairo::Vec4(0.9f, 0.4f, 0.1f, 1.0f)
        : kairo::Vec4(0.9f, 0.25f, 0.2f, 1.0f);

    m_world.add_component(e, Position{ { center.x, center.y, 0 } });
    m_world.add_component(e, Sprite{
        color, color, { sz, sz },
        static_cast<int>(kairo::RenderLayer::Default)
    });

    EnemyComp ec;
    ec.tile_x = tx; ec.tile_y = ty;
    ec.move_interval = is_strong ? 0.8f : 0.5f;
    ec.patrol_dir = m_rng() % 4;
    m_world.add_component(e, ec);

    kairo::HealthComponent hp;
    hp.current = is_strong ? 3.0f : 1.0f;
    hp.max = hp.current;
    m_world.add_component(e, hp);

    m_tags.tag(m_world, e, "enemy");
}

void DungeonScene::spawn_item(int tx, int ty, ItemComp::Type type) {
    auto e = m_world.create();
    auto center = tile_center(tx, ty);

    kairo::Vec4 color;
    float sz = 16.0f;
    switch (type) {
    case ItemComp::Key:      color = { 1.0f, 0.9f, 0.2f, 1.0f }; sz = 14.0f; break;
    case ItemComp::Potion:   color = { 0.3f, 0.9f, 0.4f, 1.0f }; sz = 14.0f; break;
    case ItemComp::Treasure: color = { 0.9f, 0.7f, 0.1f, 1.0f }; sz = 12.0f; break;
    case ItemComp::Exit:     color = { 0.4f, 0.8f, 1.0f, 1.0f }; sz = 28.0f; break;
    }

    m_world.add_component(e, Position{ { center.x, center.y, 0 } });
    m_world.add_component(e, Sprite{
        color, color, { sz, sz },
        static_cast<int>(kairo::RenderLayer::Default)
    });

    ItemComp item;
    item.type = type;
    item.tile_x = tx; item.tile_y = ty;
    item.bob_timer = static_cast<float>(m_rng() % 100) / 10.0f;
    m_world.add_component(e, item);

    std::string tag = (type == ItemComp::Exit) ? "exit" : "item";
    m_tags.tag(m_world, e, tag);
}

void DungeonScene::spawn_trap(int tx, int ty) {
    auto e = m_world.create();
    auto center = tile_center(tx, ty);

    m_world.add_component(e, Position{ { center.x, center.y, 0 } });
    m_world.add_component(e, Sprite{
        { 0.7f, 0.2f, 0.2f, 0.5f }, { 0.7f, 0.2f, 0.2f, 0.5f },
        { 24.0f, 24.0f },
        static_cast<int>(kairo::RenderLayer::Background)
    });

    TrapComp trap;
    trap.tile_x = tx; trap.tile_y = ty;
    trap.damage = 10.0f + m_floor * 5.0f;
    m_world.add_component(e, trap);

    m_tags.tag(m_world, e, "trap");
}

bool DungeonScene::is_walkable(int tx, int ty) const {
    if (tx < 0 || tx >= m_map_w || ty < 0 || ty >= m_map_h) return false;
    return m_grid[ty][tx] > 0;
}

kairo::Vec2 DungeonScene::tile_center(int tx, int ty) const {
    return { tx * m_tile_size + m_tile_size * 0.5f,
             ty * m_tile_size + m_tile_size * 0.5f };
}

void DungeonScene::show_message(const std::string& msg) {
    m_message = msg;
    m_message_timer = 2.5f;
}

void DungeonScene::try_move_player(int dx, int dy) {
    m_world.query<PlayerComp, Position>(
        [&](kairo::Entity e, PlayerComp& pc, Position& pos) {
            int nx = pc.tile_x + dx;
            int ny = pc.tile_y + dy;

            if (!is_walkable(nx, ny)) return;

            // check for enemies on target tile
            bool blocked = false;
            m_world.query<EnemyComp, kairo::HealthComponent>(
                [&](kairo::Entity enemy, EnemyComp& ec, kairo::HealthComponent& ehp) {
                    if (ec.tile_x == nx && ec.tile_y == ny) {
                        // attack enemy
                        ehp.damage(1.0f);

                        // flash white
                        if (m_world.has_component<Sprite>(enemy)) {
                            m_world.get_component<Sprite>(enemy).color = { 1, 1, 1, 1 };
                            m_world.get_component<EnemyComp>(enemy).flash_timer = 0.1f;
                        }

                        auto ec_pos = tile_center(ec.tile_x, ec.tile_y);
                        m_hit_particles.set_position(ec_pos);
                        m_hit_particles.burst(8);
                        m_cam_ctrl.add_trauma(0.15f);
                        m_audio.play("assets/sounds/hit.wav", 0.4f);

                        if (!ehp.is_alive()) {
                            m_score += 50;
                            m_tweens.tween(&m_score_display, static_cast<float>(m_score), 0.3f);
                            show_message("Enemy defeated!");

                            // death particles
                            m_hit_particles.set_position(ec_pos);
                            m_hit_particles.burst(20);
                            m_cam_ctrl.add_trauma(0.3f);

                            m_world.destroy(enemy);
                        }

                        blocked = true;
                    }
                }
            );
            if (blocked) return;

            // move player
            pc.tile_x = nx;
            pc.tile_y = ny;
            auto target = tile_center(nx, ny);

            // smooth tween to new position
            m_tweens.tween(&pos.pos.x, target.x, 0.1f);
            m_tweens.tween(&pos.pos.y, target.y, 0.1f);

            // check items on new tile
            std::vector<kairo::Entity> to_pickup;
            m_world.query<ItemComp>(
                [&](kairo::Entity item_e, ItemComp& item) {
                    if (item.tile_x == nx && item.tile_y == ny) {
                        switch (item.type) {
                        case ItemComp::Key:
                            pc.keys_collected++;
                            m_score += 100;
                            show_message("Key collected!");
                            break;
                        case ItemComp::Potion:
                            pc.potions++;
                            if (m_world.has_component<kairo::HealthComponent>(e)) {
                                m_world.get_component<kairo::HealthComponent>(e).heal(30);
                            }
                            show_message("Potion! +30 HP");
                            break;
                        case ItemComp::Treasure:
                            m_score += 250;
                            show_message("Treasure! +250");
                            break;
                        case ItemComp::Exit:
                            show_message("Descending...");
                            m_timers.after(0.5f, [this]() { next_floor(); });
                            return;
                        }

                        m_pickup_particles.set_position(target);
                        m_pickup_particles.burst(10);
                        m_tweens.tween(&m_score_display, static_cast<float>(m_score), 0.3f);
                        to_pickup.push_back(item_e);
                    }
                }
            );
            for (auto pe : to_pickup) {
                if (m_world.is_alive(pe)) m_world.destroy(pe);
            }

            // check traps
            m_world.query<TrapComp>(
                [&](kairo::Entity, TrapComp& trap) {
                    if (trap.tile_x == nx && trap.tile_y == ny && trap.armed) {
                        trap.armed = false;
                        trap.rearm_timer = 3.0f;

                        if (m_world.has_component<kairo::HealthComponent>(e) &&
                            m_world.has_component<kairo::InvincibilityComponent>(e)) {
                            auto& inv = m_world.get_component<kairo::InvincibilityComponent>(e);
                            if (!inv.is_active()) {
                                m_world.get_component<kairo::HealthComponent>(e).damage(trap.damage);
                                inv.activate();
                                m_cam_ctrl.add_trauma(0.4f);
                                show_message("Trap! Ouch!");

                                m_trap_particles.set_position(target);
                                m_trap_particles.burst(12);

                                if (!m_world.get_component<kairo::HealthComponent>(e).is_alive()) {
                                    m_game_over = true;
                                }
                            }
                        }
                    }
                }
            );

            // play step sound (reuse shoot sound pitched differently)
            m_audio.play("assets/sounds/shoot.wav", 0.1f);
        }
    );
}

void DungeonScene::next_floor() {
    m_scenes.switch_to(std::make_unique<DungeonScene>(m_scenes, m_floor + 1));
}

void DungeonScene::on_enter() {
    m_camera.set_orthographic(1280.0f, 720.0f);
    m_cam_ctrl.set_camera(&m_camera);
    m_cam_ctrl.set_smoothing(12.0f);
    m_cam_ctrl.set_dead_zone({ 10, 10 });
    m_cam_ctrl.set_shake_intensity(8.0f, 0.02f);

    m_lights.init();
    m_lights.set_ambient({ 0.12f, 0.1f, 0.15f }, 0.25f);

    m_audio.init();
    m_audio.set_master_volume(0.5f);

    m_post_fx.init(1280, 720);
    auto bloom = std::make_unique<kairo::BloomEffect>();
    bloom->set_threshold(0.5f);
    bloom->set_intensity(0.3f);
    m_post_fx.add_effect(std::move(bloom));
    auto vig = std::make_unique<kairo::VignetteEffect>();
    vig->set_intensity(0.4f);
    vig->set_softness(0.6f);
    m_post_fx.add_effect(std::move(vig));

    // particles
    kairo::ParticleEmitterConfig hit;
    hit.max_particles = 200;
    hit.emit_rate = 0; hit.looping = false;
    hit.lifetime = { 0.1f, 0.3f };
    hit.velocity = { {-180, -180}, {180, 180} };
    hit.start_size = { 2, 5 }; hit.end_size = { 0, 0 };
    hit.start_color = { {1, 0.6f, 0.2f, 1}, {1, 0.9f, 0.4f, 1} };
    hit.end_color = { {1, 0.1f, 0, 0}, {1, 0.3f, 0, 0} };
    hit.damping = 5;
    m_hit_particles.init(hit);

    kairo::ParticleEmitterConfig pickup;
    pickup.max_particles = 100;
    pickup.emit_rate = 0; pickup.looping = false;
    pickup.lifetime = { 0.2f, 0.5f };
    pickup.velocity = { {-60, -60}, {60, 60} };
    pickup.start_size = { 2, 4 }; pickup.end_size = { 0, 0 };
    pickup.start_color = { {0.3f, 1, 0.4f, 1}, {0.5f, 1, 0.6f, 1} };
    pickup.end_color = { {0.1f, 0.5f, 0.2f, 0}, {0.2f, 0.7f, 0.3f, 0} };
    pickup.damping = 4;
    m_pickup_particles.init(pickup);

    kairo::ParticleEmitterConfig trap;
    trap.max_particles = 100;
    trap.emit_rate = 0; trap.looping = false;
    trap.lifetime = { 0.1f, 0.4f };
    trap.velocity = { {-120, -120}, {120, 120} };
    trap.start_size = { 3, 6 }; trap.end_size = { 0, 0 };
    trap.start_color = { {1, 0.2f, 0.2f, 1}, {1, 0.4f, 0.3f, 1} };
    trap.end_color = { {0.5f, 0, 0, 0}, {0.8f, 0.1f, 0, 0} };
    trap.damping = 5;
    m_trap_particles.init(trap);

    generate_dungeon();
    populate_dungeon();

    show_message("Floor " + std::to_string(m_floor));

    kairo::log::info("=== DUNGEON FLOOR %d ===", m_floor);
    kairo::log::info("WASD/Arrows to move | P pause | F1 editor");
}

void DungeonScene::on_exit() {
    m_lights.shutdown();
    m_post_fx.shutdown();
    m_audio.shutdown();
}

void DungeonScene::on_fixed_update(float dt) {
    // no real-time physics needed — movement is tile-based
}

void DungeonScene::on_update(float dt) {
    KAIRO_PROFILE_SCOPE("DungeonUpdate");
    m_time += dt;

    if (m_game_over) {
        m_game_over_timer += dt;
        if (m_game_over_timer > 2.0f) {
            m_scenes.switch_to(std::make_unique<GameOverDungeonScene>(m_scenes, m_score, m_floor));
        }
        return;
    }

    // player movement — tap to move one tile, hold to auto-repeat
    m_world.query<PlayerComp>(
        [&](kairo::Entity, PlayerComp& pc) {
            pc.move_cooldown -= dt;
            if (pc.move_cooldown > 0) return;

            int dx = 0, dy = 0;
            bool was_pressed = false;

            // check single-tap first (immediate response)
            if (kairo::Input::is_key_pressed(kairo::Key::W) || kairo::Input::is_key_pressed(kairo::Key::Up))    { dy = 1; was_pressed = true; }
            if (kairo::Input::is_key_pressed(kairo::Key::S) || kairo::Input::is_key_pressed(kairo::Key::Down))  { dy = -1; was_pressed = true; }
            if (kairo::Input::is_key_pressed(kairo::Key::A) || kairo::Input::is_key_pressed(kairo::Key::Left))  { dx = -1; was_pressed = true; }
            if (kairo::Input::is_key_pressed(kairo::Key::D) || kairo::Input::is_key_pressed(kairo::Key::Right)) { dx = 1; was_pressed = true; }

            // held keys only after initial delay
            if (!was_pressed) {
                if (kairo::Input::is_key_held(kairo::Key::W) || kairo::Input::is_key_held(kairo::Key::Up))    dy = 1;
                if (kairo::Input::is_key_held(kairo::Key::S) || kairo::Input::is_key_held(kairo::Key::Down))  dy = -1;
                if (kairo::Input::is_key_held(kairo::Key::A) || kairo::Input::is_key_held(kairo::Key::Left))  dx = -1;
                if (kairo::Input::is_key_held(kairo::Key::D) || kairo::Input::is_key_held(kairo::Key::Right)) dx = 1;
            }

            if (dx != 0 || dy != 0) {
                if (dx != 0) dy = 0; // one direction per step
                try_move_player(dx, dy);
                // tap = short cooldown, hold = longer repeat rate
                pc.move_cooldown = was_pressed ? 0.15f : 0.1f;
            }
        }
    );

    // enemy AI
    {
        KAIRO_PROFILE_SCOPE("EnemyAI");

        kairo::Vec2 player_tile = { 0, 0 };
        m_world.query<PlayerComp>(
            [&](kairo::Entity, PlayerComp& pc) {
                player_tile = { static_cast<float>(pc.tile_x), static_cast<float>(pc.tile_y) };
            }
        );

        m_world.query<EnemyComp, Position, kairo::HealthComponent>(
            [&](kairo::Entity e, EnemyComp& ec, Position& pos, kairo::HealthComponent&) {
                ec.move_timer += dt;
                if (ec.move_timer < ec.move_interval) return;
                ec.move_timer = 0.0f;

                // check distance to player
                float dist = std::abs(ec.tile_x - player_tile.x) + std::abs(ec.tile_y - player_tile.y);

                int dx = 0, dy = 0;

                if (dist < 8) {
                    // chase: use pathfinding
                    auto path = m_navgrid.find_path(
                        tile_center(ec.tile_x, ec.tile_y),
                        tile_center(static_cast<int>(player_tile.x), static_cast<int>(player_tile.y))
                    );

                    if (path.size() > 1) {
                        int nx, ny;
                        m_navgrid.world_to_grid(path[1], nx, ny);
                        dx = nx - ec.tile_x;
                        dy = ny - ec.tile_y;
                        // clamp to one step
                        if (dx != 0) dx = dx > 0 ? 1 : -1;
                        if (dy != 0) dy = dy > 0 ? 1 : -1;
                        if (dx != 0) dy = 0;
                    }
                } else {
                    // patrol: walk in one direction, turn on wall
                    int dirs[4][2] = { {1,0}, {0,1}, {-1,0}, {0,-1} };
                    dx = dirs[ec.patrol_dir % 4][0];
                    dy = dirs[ec.patrol_dir % 4][1];
                    if (!is_walkable(ec.tile_x + dx, ec.tile_y + dy)) {
                        ec.patrol_dir = (ec.patrol_dir + 1) % 4;
                        dx = dirs[ec.patrol_dir][0];
                        dy = dirs[ec.patrol_dir][1];
                    }
                }

                int nx = ec.tile_x + dx;
                int ny = ec.tile_y + dy;

                if (is_walkable(nx, ny)) {
                    // check if player is there — damage player
                    bool hit_player = false;
                    m_world.query<PlayerComp, kairo::HealthComponent, kairo::InvincibilityComponent>(
                        [&](kairo::Entity, PlayerComp& pc, kairo::HealthComponent& php, kairo::InvincibilityComponent& inv) {
                            if (pc.tile_x == nx && pc.tile_y == ny) {
                                hit_player = true;
                                if (!inv.is_active()) {
                                    php.damage(15.0f + m_floor * 3.0f);
                                    inv.activate();
                                    m_cam_ctrl.add_trauma(0.35f);
                                    show_message("Hit! -" + std::to_string(static_cast<int>(15 + m_floor * 3)) + " HP");

                                    if (!php.is_alive()) {
                                        m_game_over = true;
                                    }
                                }
                            }
                        }
                    );

                    // check if another enemy is there
                    bool occupied = false;
                    m_world.query<EnemyComp>(
                        [&](kairo::Entity other, EnemyComp& oec) {
                            if (other != e && oec.tile_x == nx && oec.tile_y == ny) {
                                occupied = true;
                            }
                        }
                    );

                    if (!occupied) {
                        ec.tile_x = nx;
                        ec.tile_y = ny;
                        auto target = tile_center(nx, ny);
                        m_tweens.tween(&pos.pos.x, target.x, 0.15f);
                        m_tweens.tween(&pos.pos.y, target.y, 0.15f);
                    }
                }
            }
        );
    }

    // enemy flash decay
    m_world.query<Sprite, EnemyComp>(
        [&](kairo::Entity, Sprite& s, EnemyComp& ec) {
            if (ec.flash_timer > 0) {
                ec.flash_timer -= dt;
                if (ec.flash_timer <= 0) s.color = s.base_color;
            }
        }
    );

    // trap rearming
    m_world.query<TrapComp, Sprite>(
        [&](kairo::Entity, TrapComp& trap, Sprite& s) {
            if (!trap.armed) {
                trap.rearm_timer -= dt;
                s.color.w = 0.2f;
                if (trap.rearm_timer <= 0) {
                    trap.armed = true;
                    s.color.w = 0.5f;
                }
            }
        }
    );

    // invincibility update
    m_world.query<kairo::InvincibilityComponent, Sprite, PlayerComp>(
        [&](kairo::Entity, kairo::InvincibilityComponent& inv, Sprite& s, PlayerComp&) {
            inv.update(dt);
            if (inv.is_active()) {
                s.color.w = std::sin(m_time * 20) > 0 ? 1.0f : 0.3f;
            } else {
                s.color.w = 1.0f;
            }
        }
    );

    // item bobbing
    m_world.query<ItemComp, Position>(
        [&](kairo::Entity, ItemComp& item, Position& pos) {
            item.bob_timer += dt;
            float bob = std::sin(item.bob_timer * 2.5f) * 2.0f;
            auto base = tile_center(item.tile_x, item.tile_y);
            pos.pos.y = base.y + bob;

            if (item.type == ItemComp::Exit) {
                pos.rotation += dt * 1.5f;
            }
        }
    );

    // camera follow player
    m_world.query<Position, PlayerComp>(
        [&](kairo::Entity, Position& pos, PlayerComp&) {
            m_cam_ctrl.set_target({ pos.pos.x, pos.pos.y });
        }
    );

    // message timer
    if (m_message_timer > 0) m_message_timer -= dt;

    // pause
    if (kairo::Input::is_key_pressed(kairo::Key::P)) {
        m_scenes.push(std::make_unique<TitleScene>(m_scenes));
    }

    // use potion with Q
    if (kairo::Input::is_key_pressed(kairo::Key::Q)) {
        m_world.query<PlayerComp, kairo::HealthComponent>(
            [&](kairo::Entity, PlayerComp& pc, kairo::HealthComponent& hp) {
                if (pc.potions > 0 && hp.current < hp.max) {
                    pc.potions--;
                    hp.heal(40);
                    show_message("Used potion! +40 HP");
                    m_pickup_particles.set_position(tile_center(pc.tile_x, pc.tile_y));
                    m_pickup_particles.burst(8);
                }
            }
        );
    }

    m_cam_ctrl.update(dt);
    m_timers.update(dt);
    m_tweens.update(dt);
    m_hit_particles.update(dt);
    m_pickup_particles.update(dt);
    m_trap_particles.update(dt);
    m_audio.update();
}

void DungeonScene::on_render() {
    KAIRO_PROFILE_SCOPE("DungeonRender");

    if (m_post_fx.has_effects()) m_post_fx.begin_capture();

    kairo::Renderer::begin(m_camera);

    // tilemap
    kairo::Renderer::set_layer(kairo::RenderLayer::Background);
    auto cam_pos = m_cam_ctrl.get_position();
    m_tilemap.render_colored(cam_pos, { 700, 400 });

    // entities
    m_world.query<Position, Sprite>(
        [](kairo::Entity, Position& pos, Sprite& s) {
            kairo::Renderer::set_layer(s.layer);
            kairo::Renderer::draw_quad(pos.pos, s.size, pos.rotation, s.color);
        }
    );

    // particles
    kairo::Renderer::set_layer(kairo::RenderLayer::Foreground);
    m_hit_particles.render(m_camera);
    m_pickup_particles.render(m_camera);
    m_trap_particles.render(m_camera);

    kairo::Renderer::end();

    // lighting
    m_lights.clear_lights();
    m_world.query<Position, PlayerComp>(
        [&](kairo::Entity, Position& pos, PlayerComp&) {
            m_lights.add_light({ { pos.pos.x, pos.pos.y }, { 0.9f, 0.85f, 0.7f }, 2.0f, 350.0f });
        }
    );
    // enemy glows (dim red)
    m_world.query<Position, EnemyComp>(
        [&](kairo::Entity, Position& pos, EnemyComp&) {
            m_lights.add_light({ { pos.pos.x, pos.pos.y }, { 0.8f, 0.2f, 0.1f }, 0.3f, 50.0f });
        }
    );
    // item glows
    m_world.query<Position, ItemComp>(
        [&](kairo::Entity, Position& pos, ItemComp& item) {
            kairo::Vec3 glow;
            float intensity = 0.4f, radius = 40.0f;
            switch (item.type) {
            case ItemComp::Key:      glow = { 1, 0.9f, 0.2f }; break;
            case ItemComp::Potion:   glow = { 0.2f, 0.9f, 0.3f }; break;
            case ItemComp::Treasure: glow = { 0.9f, 0.7f, 0.1f }; break;
            case ItemComp::Exit:     glow = { 0.3f, 0.7f, 1.0f }; intensity = 0.8f; radius = 80.0f; break;
            }
            m_lights.add_light({ { pos.pos.x, pos.pos.y }, glow, intensity, radius });
        }
    );
    m_lights.render(m_camera);

    if (m_post_fx.has_effects()) m_post_fx.end_and_apply();

    // debug draw
    kairo::DebugDraw::render(m_camera);

    // === HUD ===
    kairo::UI::begin(1280, 720);

    // health bar
    float hp_ratio = 1.0f;
    int potions = 0, keys = 0;
    m_world.query<kairo::HealthComponent, PlayerComp>(
        [&](kairo::Entity, kairo::HealthComponent& hp, PlayerComp& pc) {
            hp_ratio = hp.ratio();
            potions = pc.potions;
            keys = pc.keys_collected;
        }
    );

    kairo::Vec4 hp_color = hp_ratio > 0.5f ? kairo::Vec4(0.2f, 0.85f, 0.3f, 1)
                         : hp_ratio > 0.25f ? kairo::Vec4(0.9f, 0.8f, 0.2f, 1)
                         : kairo::Vec4(0.9f, 0.2f, 0.2f, 1);
    kairo::UI::progress_bar({ 20, 20 }, { 200, 18 }, hp_ratio, hp_color);
    kairo::UI::label({ 20, 42 }, "HP", 1.5f, { 0.7f, 0.7f, 0.8f, 0.8f });

    // score
    char buf[64];
    snprintf(buf, sizeof(buf), "SCORE %d", static_cast<int>(m_score_display));
    kairo::UI::label({ 20, 65 }, buf, 2.0f, { 0.9f, 0.95f, 1, 1 });

    // floor
    snprintf(buf, sizeof(buf), "FLOOR %d", m_floor);
    kairo::UI::label({ 20, 90 }, buf, 1.5f, { 0.5f, 0.6f, 0.8f, 0.8f });

    // inventory
    snprintf(buf, sizeof(buf), "Keys: %d  Potions: %d [Q]", keys, potions);
    kairo::UI::label({ 20, 110 }, buf, 1.5f, { 0.8f, 0.8f, 0.6f, 0.8f });

    // message
    if (m_message_timer > 0) {
        float alpha = std::min(1.0f, m_message_timer);
        kairo::UI::label_centered({ 640, 550 }, m_message, 2.5f,
            { 1, 1, 1, alpha });
    }

    // game over overlay
    if (m_game_over) {
        float fade = std::min(m_game_over_timer / 1.5f, 1.0f);
        kairo::UI::panel({ 0, 0 }, { 1280, 720 }, { 0.05f, 0, 0.02f, fade * 0.7f });
        if (m_game_over_timer > 0.5f) {
            kairo::UI::label_centered({ 640, 300 }, "YOU DIED", 3.5f, { 1, 0.3f, 0.2f, 1 });
        }
    }

    kairo::UI::end();
}

// ==================== TitleScene (used as pause) ====================

TitleScene::TitleScene(kairo::SceneManager& scenes) : Scene("Title"), m_scenes(scenes) {}

void TitleScene::on_enter() {
    m_camera.set_orthographic(1280, 720);
}

void TitleScene::on_update(float dt) {
    m_time += dt;
    if (kairo::Input::is_key_pressed(kairo::Key::P) ||
        kairo::Input::is_key_pressed(kairo::Key::Escape)) {
        m_scenes.pop();
    }
}

void TitleScene::on_render() {
    kairo::UI::begin(1280, 720);
    kairo::UI::panel({ 0, 0 }, { 1280, 720 }, { 0, 0, 0.03f, 0.8f });
    kairo::UI::label_centered({ 640, 310 }, "PAUSED", 3.0f, { 0.8f, 0.85f, 1, 1 });

    float pulse = (std::sin(m_time * 2.5f) + 1) * 0.5f;
    kairo::UI::label_centered({ 640, 360 }, "press P to resume", 2.0f,
        { 0.5f, 0.5f, 0.6f, 0.4f + pulse * 0.6f });
    kairo::UI::end();
}

// ==================== GameOverDungeonScene ====================

GameOverDungeonScene::GameOverDungeonScene(kairo::SceneManager& scenes, int score, int floor)
    : Scene("GameOver"), m_scenes(scenes), m_score(score), m_floor(floor) {}

void GameOverDungeonScene::on_enter() {
    m_camera.set_orthographic(1280, 720);
}

void GameOverDungeonScene::on_update(float dt) {
    m_time += dt;
    if (kairo::Input::is_key_pressed(kairo::Key::R)) {
        m_scenes.switch_to(std::make_unique<DungeonScene>(m_scenes, 1));
    }
}

void GameOverDungeonScene::on_render() {
    // render a solid background quad so there's no leftover frame garbage
    kairo::Renderer::begin(m_camera);
    kairo::Renderer::set_layer(kairo::RenderLayer::Background);
    kairo::Renderer::draw_quad(
        kairo::Vec3(0, 0, 0), kairo::Vec2(1400, 800),
        kairo::Vec4(0.03f, 0.01f, 0.05f, 1.0f)
    );
    kairo::Renderer::end();

    kairo::UI::begin(1280, 720);

    kairo::UI::label_centered({ 640, 240 }, "GAME OVER", 3.5f, { 1, 0.3f, 0.2f, 1 });

    char buf[64];
    snprintf(buf, sizeof(buf), "Score: %d", m_score);
    kairo::UI::label_centered({ 640, 320 }, buf, 2.5f, { 0.9f, 0.95f, 1, 1 });

    snprintf(buf, sizeof(buf), "Reached Floor %d", m_floor);
    kairo::UI::label_centered({ 640, 360 }, buf, 2.0f, { 0.6f, 0.7f, 0.9f, 0.9f });

    float pulse = (std::sin(m_time * 2) + 1) * 0.5f;
    kairo::UI::label_centered({ 640, 430 }, "press R to restart", 2.0f,
        { 0.5f, 0.5f, 0.7f, 0.4f + pulse * 0.6f });
    kairo::UI::end();
}

// ==================== DungeonGame (Application) ====================

void DungeonGame::on_init() {
    m_use_scenes = true;
    m_scenes.push(std::make_unique<DungeonScene>(m_scenes, 1));
}

void DungeonGame::on_shutdown() {
    kairo::log::info("dungeon game shutting down");
}

void DungeonGame::on_editor_ui(float fps, float dt, const kairo::Renderer::Stats& stats) {
    auto* active = m_scenes.get_active();
    if (!active) return;
    m_stats.draw(fps, dt, stats, active->get_world().entity_count());
    m_profiler.draw(fps, dt);
}
