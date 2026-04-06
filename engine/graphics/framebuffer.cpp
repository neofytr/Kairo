#include "graphics/framebuffer.h"
#include "core/log.h"

#include <glad/glad.h>

namespace kairo {

Framebuffer::~Framebuffer() {
    shutdown();
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_fbo(other.m_fbo)
    , m_color_texture(other.m_color_texture)
    , m_depth_rbo(other.m_depth_rbo)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_has_depth(other.m_has_depth)
{
    other.m_fbo           = 0;
    other.m_color_texture = 0;
    other.m_depth_rbo     = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
    if (this != &other) {
        shutdown();
        m_fbo           = other.m_fbo;
        m_color_texture = other.m_color_texture;
        m_depth_rbo     = other.m_depth_rbo;
        m_width         = other.m_width;
        m_height        = other.m_height;
        m_has_depth     = other.m_has_depth;
        other.m_fbo           = 0;
        other.m_color_texture = 0;
        other.m_depth_rbo     = 0;
    }
    return *this;
}

bool Framebuffer::init(const FramebufferConfig& config) {
    m_width    = config.width;
    m_height   = config.height;
    m_has_depth = config.depth;

    return create_attachments();
}

void Framebuffer::shutdown() {
    delete_attachments();
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void Framebuffer::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resize(i32 width, i32 height) {
    if (width == m_width && height == m_height) return;
    m_width  = width;
    m_height = height;
    delete_attachments();
    create_attachments();
}

bool Framebuffer::create_attachments() {
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // color attachment (RGBA8)
    glGenTextures(1, &m_color_texture);
    glBindTexture(GL_TEXTURE_2D, m_color_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_color_texture, 0);

    // optional depth renderbuffer
    if (m_has_depth) {
        glGenRenderbuffers(1, &m_depth_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depth_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                              m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, m_depth_rbo);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        log::error("framebuffer: incomplete (%dx%d)", m_width, m_height);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void Framebuffer::delete_attachments() {
    if (m_color_texture != 0) {
        glDeleteTextures(1, &m_color_texture);
        m_color_texture = 0;
    }
    if (m_depth_rbo != 0) {
        glDeleteRenderbuffers(1, &m_depth_rbo);
        m_depth_rbo = 0;
    }
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
}

} // namespace kairo
