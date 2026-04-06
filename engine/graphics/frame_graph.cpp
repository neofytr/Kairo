#include "graphics/frame_graph.h"
#include "core/log.h"

#include <glad/glad.h>
#include <algorithm>

namespace kairo {

void FrameGraph::add_pass(const RenderPass& pass) {
    m_passes.push_back(pass);
    m_compiled = false;
}

void FrameGraph::add_screen_pass(const std::string& name,
                                 const std::vector<std::string>& inputs,
                                 std::function<void(const std::unordered_map<std::string, u32>&)> execute) {
    RenderPass pass;
    pass.name    = name;
    pass.inputs  = inputs;
    pass.output  = ""; // empty = render to screen
    pass.execute = std::move(execute);
    m_passes.push_back(std::move(pass));
    m_compiled = false;
}

bool FrameGraph::compile() {
    // Shutdown any previous compilation artifacts
    shutdown();

    // Track which outputs are available as we walk the pass list
    std::unordered_map<std::string, bool> available;

    for (auto& pass : m_passes) {
        // Validate that all inputs reference outputs from earlier passes
        for (const auto& input : pass.inputs) {
            if (available.find(input) == available.end()) {
                log::error("FrameGraph: pass '%s' requires input '%s' which is not produced by any earlier pass",
                           pass.name.c_str(), input.c_str());
                return false;
            }
        }

        // Create a framebuffer for passes that have an output
        if (!pass.output.empty()) {
            auto fb = std::make_unique<Framebuffer>();
            if (!fb->init(pass.output_config)) {
                log::error("FrameGraph: failed to create framebuffer for pass '%s'", pass.name.c_str());
                return false;
            }
            m_textures[pass.output] = fb->get_color_attachment();
            m_framebuffers[pass.output] = std::move(fb);
            available[pass.output] = true;
        }
    }

    m_compiled = true;
    log::info("FrameGraph: compiled %zu passes", m_passes.size());
    return true;
}

void FrameGraph::execute() {
    if (!m_compiled) {
        log::error("FrameGraph: must compile before executing");
        return;
    }

    for (auto& pass : m_passes) {
        // Bind output framebuffer, or default FBO for screen passes
        if (!pass.output.empty()) {
            auto it = m_framebuffers.find(pass.output);
            if (it != m_framebuffers.end()) {
                it->second->bind();
            }
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // Build the input texture map for this pass
        std::unordered_map<std::string, u32> input_textures;
        for (const auto& input : pass.inputs) {
            auto it = m_textures.find(input);
            if (it != m_textures.end()) {
                input_textures[input] = it->second;
            }
        }

        // Execute the pass
        if (pass.execute) {
            pass.execute(input_textures);
        }

        // Unbind framebuffer
        if (!pass.output.empty()) {
            auto it = m_framebuffers.find(pass.output);
            if (it != m_framebuffers.end()) {
                it->second->unbind();
            }
        }
    }
}

void FrameGraph::resize(i32 width, i32 height) {
    for (auto& [name, fb] : m_framebuffers) {
        fb->resize(width, height);
    }

    // Update cached texture IDs after resize (attachments may be recreated)
    for (auto& [name, fb] : m_framebuffers) {
        m_textures[name] = fb->get_color_attachment();
    }
}

void FrameGraph::shutdown() {
    for (auto& [name, fb] : m_framebuffers) {
        fb->shutdown();
    }
    m_framebuffers.clear();
    m_textures.clear();
    m_compiled = false;
}

u32 FrameGraph::get_texture(const std::string& name) const {
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        return it->second;
    }
    log::warn("FrameGraph: texture '%s' not found", name.c_str());
    return 0;
}

size_t FrameGraph::pass_count() const {
    return m_passes.size();
}

const std::vector<RenderPass>& FrameGraph::get_passes() const {
    return m_passes;
}

} // namespace kairo
