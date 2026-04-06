#pragma once

#include "graphics/post_process.h"
#include "graphics/framebuffer.h"
#include "graphics/shader.h"
#include "core/types.h"

namespace kairo {

class VignetteEffect : public PostProcessEffect {
public:
    bool init(i32 width, i32 height) override;
    void shutdown() override;
    void apply(u32 input_texture, Framebuffer& output) override;
    const char* get_name() const override { return "vignette"; }

    void set_intensity(f32 intensity) { m_intensity = intensity; }
    void set_softness(f32 softness) { m_softness = softness; }

private:
    Shader m_shader;

    f32 m_intensity = 0.4f;
    f32 m_softness  = 0.5f;

    u32 m_quad_vao = 0;
    u32 m_quad_vbo = 0;

    void setup_quad();
    void draw_quad();
};

} // namespace kairo
