#version 460 core

in vec4 v_color;
in vec2 v_tex_coords;
in float v_tex_index;

out vec4 frag_color;

// support up to 16 texture slots per batch
uniform sampler2D u_textures[16];

void main() {
    int index = int(v_tex_index);
    vec4 tex_color = texture(u_textures[index], v_tex_coords);
    frag_color = tex_color * v_color;
}
