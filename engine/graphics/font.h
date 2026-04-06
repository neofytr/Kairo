#pragma once

#include "core/types.h"
#include "graphics/texture.h"
#include "math/vec2.h"

#include <string>
#include <unordered_map>

namespace kairo {

struct Glyph {
    Vec2 uv_min;      // top-left UV in atlas
    Vec2 uv_max;      // bottom-right UV in atlas
    Vec2 size;         // pixel size of glyph
    Vec2 offset;       // offset from cursor to glyph origin
    float advance;     // horizontal advance after this glyph
};

class Font {
public:
    Font() = default;
    ~Font() = default;

    Font(Font&&) = default;
    Font& operator=(Font&&) = default;
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    // load a BMFont text format .fnt file + its atlas texture
    bool load_bmfont(const std::string& fnt_path);

    // create a built-in minimal bitmap font (no files needed)
    bool create_default();

    const Glyph* get_glyph(char c) const;
    const Texture& get_texture() const { return m_atlas; }
    float get_line_height() const { return m_line_height; }

    // measure the pixel width of a string at a given scale
    float measure_width(const std::string& text, float scale = 1.0f) const;

private:
    Texture m_atlas;
    std::unordered_map<char, Glyph> m_glyphs;
    float m_line_height = 16.0f;

    bool parse_fnt(const std::string& path);
};

} // namespace kairo
