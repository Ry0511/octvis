#version 450

in vec4 col;
in vec2 tex_coord;
out vec4 color;

//uniform sampler2D img;

void main() {
//    vec4 colour = mix(texture(img, tex_coord), col, 0.5);
    color = col;
}