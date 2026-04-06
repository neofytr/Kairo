#pragma once

#include "core/types.h"
#include "math/mat4.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include <string>

namespace kairo {

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept : m_id(other.m_id) { other.m_id = 0; }
    Shader& operator=(Shader&& other) noexcept;

    // load from file paths
    bool load(const std::string& vert_path, const std::string& frag_path);

    // load from source strings directly
    bool load_from_source(const std::string& vert_src, const std::string& frag_src);

    void bind() const;
    void unbind() const;

    // uniform setters
    void set_int(const char* name, int value) const;
    void set_float(const char* name, float value) const;
    void set_vec3(const char* name, const Vec3& value) const;
    void set_vec4(const char* name, const Vec4& value) const;
    void set_mat4(const char* name, const Mat4& value) const;

    u32 get_id() const { return m_id; }

private:
    u32 m_id = 0;

    static u32 compile_shader(u32 type, const std::string& source);
    static std::string read_file(const std::string& path);
    int get_location(const char* name) const;
};

} // namespace kairo
