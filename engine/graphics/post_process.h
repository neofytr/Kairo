#pragma once

#include "core/types.h"
#include "graphics/framebuffer.h"
#include "graphics/shader.h"

#include <memory>
#include <vector>

namespace kairo {

// base class for all post-processing effects
class PostProcessEffect {
public:
    virtual ~PostProcessEffect() = default;
    virtual bool init(i32 width, i32 height) = 0;
    virtual void shutdown() = 0;
    // read from input_texture, write to output framebuffer
    virtual void apply(u32 input_texture, Framebuffer& output) = 0;
    virtual const char* get_name() const = 0;
};

// manages a chain of post-process effects
class PostProcessStack {
public:
    bool init(i32 width, i32 height);
    void shutdown();

    void add_effect(std::unique_ptr<PostProcessEffect> effect);

    void begin_capture();     // bind scene fbo
    void end_and_apply();     // run effects, blit final to screen
    void resize(i32 width, i32 height);

    bool has_effects() const { return !m_effects.empty(); }

    // access the fullscreen quad for effects to reuse
    u32 get_quad_vao() const { return m_quad_vao; }

private:
    Framebuffer m_scene_fbo;
    Framebuffer m_ping;
    Framebuffer m_pong;

    Shader m_blit_shader;
    u32 m_quad_vao = 0;
    u32 m_quad_vbo = 0;

    i32 m_width  = 0;
    i32 m_height = 0;

    std::vector<std::unique_ptr<PostProcessEffect>> m_effects;

    void draw_fullscreen_quad();
    void blit_to_screen(u32 texture);
};

} // namespace kairo
