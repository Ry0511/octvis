#version 450

in vec3 out_colour;
out vec4 color;

void main() {
    color = vec4(out_colour, 1.0);
}