#pragma once

#include "assets/asset_manager.h"
#include "graphics/shader.h"

#include <optional>
#include <string>

namespace kairo {

// shader loader — expects path format "base_path" and loads base_path.vert + base_path.frag
inline AssetManager<Shader>::LoaderFunc make_shader_loader() {
    return [](const std::string& base_path) -> std::optional<Shader> {
        Shader shader;
        std::string vert = base_path + ".vert";
        std::string frag = base_path + ".frag";
        if (!shader.load(vert, frag)) {
            return std::nullopt;
        }
        return std::move(shader);
    };
}

} // namespace kairo
