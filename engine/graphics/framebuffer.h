#pragma once

#include "core/types.h"

namespace kairo {

struct FramebufferConfig {
    i32 width  = 800;
    i32 height = 600;
    bool depth = false;
};

class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer();

    // non-copyable
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    // movable
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    bool init(const FramebufferConfig& config);
    void shutdown();

    void bind() const;
    void unbind() const;

    void resize(i32 width, i32 height);

    u32 get_color_attachment() const { return m_color_texture; }
    i32 get_width() const { return m_width; }
    i32 get_height() const { return m_height; }

private:
    u32 m_fbo           = 0;
    u32 m_color_texture = 0;
    u32 m_depth_rbo     = 0;
    i32 m_width         = 0;
    i32 m_height        = 0;
    bool m_has_depth    = false;

    bool create_attachments();
    void delete_attachments();
};

} // namespace kairo
