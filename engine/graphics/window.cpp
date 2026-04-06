#include "graphics/window.h"
#include "core/log.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace kairo {

Window::~Window() {
    shutdown();
}

bool Window::init(const WindowConfig& config) {
    if (!glfwInit()) {
        log::error("failed to initialize GLFW");
        return false;
    }

    // request OpenGL 4.6 core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);
    if (!m_window) {
        log::error("failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(config.vsync ? 1 : 0);

    // load OpenGL function pointers via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        log::error("failed to initialize GLAD");
        return false;
    }

    m_width  = config.width;
    m_height = config.height;

    // handle window resize
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* win, int w, int h) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(win));
        self->m_width  = w;
        self->m_height = h;
        glViewport(0, 0, w, h);
    });

    glViewport(0, 0, m_width, m_height);

    log::info("window created: %s (%dx%d)", config.title.c_str(), m_width, m_height);
    log::info("OpenGL %s | GLSL %s", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

    return true;
}

void Window::shutdown() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        glfwTerminate();
        log::info("window destroyed");
    }
}

void Window::poll_events() {
    glfwPollEvents();
}

void Window::swap_buffers() {
    glfwSwapBuffers(m_window);
}

bool Window::should_close() const {
    return glfwWindowShouldClose(m_window);
}

} // namespace kairo
