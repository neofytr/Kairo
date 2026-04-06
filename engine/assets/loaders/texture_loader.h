#pragma once

#include "assets/asset_manager.h"
#include "graphics/texture.h"

#include <optional>
#include <string>

namespace kairo {

// factory function that creates a loader for the texture asset manager
inline AssetManager<Texture>::LoaderFunc make_texture_loader(TextureConfig config = {}) {
    return [config](const std::string& path) -> std::optional<Texture> {
        Texture tex;
        if (!tex.load(path, config)) {
            return std::nullopt;
        }
        return std::move(tex);
    };
}

} // namespace kairo
