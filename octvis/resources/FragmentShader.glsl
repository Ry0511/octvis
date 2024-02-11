#version 450

in vec3 col;
in vec2 tex_coord;
out vec4 color;

uniform sampler2D img;

void main() {
    vec4 colour = mix(texture(img, tex_coord), vec4(col, 1.0), 0.5);
    color = colour;
}