#include "graphics/texture.h"
#include "core/log.h"

#include <glad/glad.h>
#include <stb_image.h>

namespace kairo {

Texture::~Texture() {
    release();
}

Texture::Texture(Texture&& other) noexcept
    : m_id(other.m_id), m_width(other.m_width), m_height(other.m_height),
      m_channels(other.m_channels), m_path(std::move(other.m_path)) {
    other.m_id = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        release();
        m_id = other.m_id;
        m_width = other.m_width;
        m_height = other.m_height;
        m_channels = other.m_channels;
        m_path = std::move(other.m_path);
        other.m_id = 0;
    }
    return *this;
}

bool Texture::load(const std::string& path, const TextureConfig& config) {
    stbi_set_flip_vertically_on_load(config.flip_y);

    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 0);
    if (!data) {
        log::error("failed to load texture: %s", path.c_str());
        return false;
    }

    // determine format from channel count
    GLenum internal_format, data_format;
    if (m_channels == 4) {
        internal_format = GL_RGBA8;
        data_format = GL_RGBA;
    } else if (m_channels == 3) {
        internal_format = GL_RGB8;
        data_format = GL_RGB;
    } else if (m_channels == 1) {
        internal_format = GL_R8;
        data_format = GL_RED;
    } else {
        log::error("unsupported channel count %d in texture: %s", m_channels, path.c_str());
        stbi_image_free(data);
        return false;
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    // filtering
    GLenum gl_filter = (config.filter == TextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter);

    // wrapping
    GLenum gl_wrap = (config.wrap == TextureWrap::Repeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_wrap);

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, m_width, m_height,
                 0, data_format, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    m_path = path;

    log::info("loaded texture: %s (%dx%d, %d channels)", path.c_str(), m_width, m_height, m_channels);
    return true;
}

bool Texture::create_white() {
    u32 white_pixel = 0xFFFFFFFF;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white_pixel);

    m_width = 1;
    m_height = 1;
    m_channels = 4;
    m_path = "<white>";

    return true;
}

void Texture::bind(u32 slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::release() {
    if (m_id) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}

} // namespace kairo
