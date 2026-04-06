#pragma once

#include "graphics/post_process.h"
#include "graphics/framebuffer.h"
#include "graphics/shader.h"
#include "core/types.h"

namespace kairo {

class BloomEffect : public PostProcessEffect {
public:
    bool init(i32 width, i32 height) override;
    void shutdown() override;
    void apply(u32 input_texture, Framebuffer& output) override;
    const char* get_name() const override { return "bloom"; }

    void set_threshold(f32 threshold) { m_threshold = threshold; }
    void set_intensity(f32 intensity) { m_intensity = intensity; }

private:
    Shader m_extract_shader;
    Shader m_blur_shader;
    Shader m_composite_shader;

    // half-res fbos for the bloom pipeline
    Framebuffer m_bright_fbo;
    Framebuffer m_blur_h_fbo;
    Framebuffer m_blur_v_fbo;

    f32 m_threshold = 0.7f;
    f32 m_intensity = 0.5f;

    i32 m_width  = 0;
    i32 m_height = 0;

    u32 m_quad_vao = 0;
    u32 m_quad_vbo = 0;

    void setup_quad();
    void draw_quad();
};

} // namespace kairo
