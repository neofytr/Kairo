#pragma once

#include "core/types.h"
#include <string>

namespace kairo {

enum class TextureFilter {
    Nearest, // pixel art, no smoothing
    Linear,  // smooth scaling
};

enum class TextureWrap {
    Repeat,
    ClampToEdge,
};

struct TextureConfig {
    TextureFilter filter = TextureFilter::Nearest;
    TextureWrap wrap = TextureWrap::ClampToEdge;
    bool flip_y = true; // most image formats have origin at top-left
};

class Texture {
public:
    Texture() = default;
    ~Texture();

    // prevent copying — GPU resources shouldn't be duplicated implicitly
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // move is fine
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool load(const std::string& path, const TextureConfig& config = {});

    // create a 1x1 white texture (used as default when no texture is bound)
    bool create_white();

    // create from raw RGBA pixel data
    bool create_from_rgba(const u8* data, i32 width, i32 height);

    void bind(u32 slot = 0) const;
    void unbind() const;

    u32 get_id() const { return m_id; }
    i32 get_width() const { return m_width; }
    i32 get_height() const { return m_height; }
    const std::string& get_path() const { return m_path; }

private:
    void release();

    u32 m_id = 0;
    i32 m_width = 0;
    i32 m_height = 0;
    i32 m_channels = 0;
    std::string m_path;
};

} // namespace kairo
