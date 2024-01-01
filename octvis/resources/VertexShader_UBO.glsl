#version 450

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 colour;
layout (location = 3) in vec2 tex;

layout(std140) uniform render_state {
    mat4 projection;
    mat4 view;
    mat4 model;
};

out vec3 col;
out vec2 tex_coord;

void main() {
    gl_Position = projection * view * model * vec4(vertex, 1.0);
    col = colour;
    tex_coord = tex;
}