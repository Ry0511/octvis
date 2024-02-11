#version 450

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 colour;

layout(std140) uniform render_config {
    mat4 projection;
    mat4 view;
    mat4 model;
};

out vec3 out_colour;

void main() {
    gl_Position = projection * view * model * vec4(vertex, 1.0);
    out_colour = colour;
}