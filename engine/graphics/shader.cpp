#include "graphics/shader.h"
#include "core/log.h"

#include <glad/glad.h>
#include <fstream>
#include <sstream>

namespace kairo {

Shader::~Shader() {
    if (m_id) {
        glDeleteProgram(m_id);
        m_id = 0;
    }
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_id) glDeleteProgram(m_id);
        m_id = other.m_id;
        other.m_id = 0;
    }
    return *this;
}

bool Shader::load(const std::string& vert_path, const std::string& frag_path) {
    std::string vert_src = read_file(vert_path);
    std::string frag_src = read_file(frag_path);

    if (vert_src.empty() || frag_src.empty()) {
        log::error("failed to read shader files: %s, %s", vert_path.c_str(), frag_path.c_str());
        return false;
    }

    return load_from_source(vert_src, frag_src);
}

bool Shader::load_from_source(const std::string& vert_src, const std::string& frag_src) {
    u32 vert = compile_shader(GL_VERTEX_SHADER, vert_src);
    u32 frag = compile_shader(GL_FRAGMENT_SHADER, frag_src);

    if (!vert || !frag) {
        if (vert) glDeleteShader(vert);
        if (frag) glDeleteShader(frag);
        return false;
    }

    m_id = glCreateProgram();
    glAttachShader(m_id, vert);
    glAttachShader(m_id, frag);
    glLinkProgram(m_id);

    int success;
    glGetProgramiv(m_id, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(m_id, 512, nullptr, info);
        log::error("shader link failed: %s", info);
        glDeleteProgram(m_id);
        m_id = 0;
    }

    // shaders are linked, no longer needed individually
    glDeleteShader(vert);
    glDeleteShader(frag);

    return m_id != 0;
}

void Shader::bind() const {
    glUseProgram(m_id);
}

void Shader::unbind() const {
    glUseProgram(0);
}

void Shader::set_int(const char* name, int value) const {
    glUniform1i(get_location(name), value);
}

void Shader::set_float(const char* name, float value) const {
    glUniform1f(get_location(name), value);
}

void Shader::set_vec3(const char* name, const Vec3& value) const {
    glUniform3f(get_location(name), value.x, value.y, value.z);
}

void Shader::set_vec4(const char* name, const Vec4& value) const {
    glUniform4f(get_location(name), value.x, value.y, value.z, value.w);
}

void Shader::set_mat4(const char* name, const Mat4& value) const {
    glUniformMatrix4fv(get_location(name), 1, GL_FALSE, value.data());
}

u32 Shader::compile_shader(u32 type, const std::string& source) {
    u32 shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, nullptr, info);
        const char* type_str = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
        log::error("%s shader compilation failed: %s", type_str, info);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

std::string Shader::read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        log::error("could not open file: %s", path.c_str());
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

int Shader::get_location(const char* name) const {
    return glGetUniformLocation(m_id, name);
}

} // namespace kairo
