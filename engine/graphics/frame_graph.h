#pragma once

#include "core/types.h"
#include "graphics/framebuffer.h"

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>

namespace kairo {

// A render pass in the frame graph
struct RenderPass {
    std::string name;

    // Resources this pass reads from (texture IDs from previous passes)
    std::vector<std::string> inputs;

    // Resource this pass writes to (empty string = screen pass)
    std::string output;

    // The actual render function; receives a map of input name -> GL texture ID
    std::function<void(const std::unordered_map<std::string, u32>& textures)> execute;

    // Framebuffer config for this pass's output
    FramebufferConfig output_config = { 1280, 720, false };
};

class FrameGraph {
public:
    // Add a render pass with its own output framebuffer
    void add_pass(const RenderPass& pass);

    // Add a pass that writes directly to the default framebuffer (screen)
    void add_screen_pass(const std::string& name,
                         const std::vector<std::string>& inputs,
                         std::function<void(const std::unordered_map<std::string, u32>&)> execute);

    // Compile the graph: validate dependencies, create framebuffers
    bool compile();

    // Execute all passes in order
    void execute();

    // Resize all framebuffers
    void resize(i32 width, i32 height);

    void shutdown();

    // Get a pass's output texture by name
    u32 get_texture(const std::string& name) const;

    size_t pass_count() const;
    const std::vector<RenderPass>& get_passes() const;

private:
    std::vector<RenderPass> m_passes;
    std::unordered_map<std::string, std::unique_ptr<Framebuffer>> m_framebuffers;
    std::unordered_map<std::string, u32> m_textures; // output name -> GL texture ID
    bool m_compiled = false;
};

} // namespace kairo
