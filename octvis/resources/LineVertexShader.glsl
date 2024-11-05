#version 450

layout (location = 0) in vec3 aVertex;
layout (location = 1) in float aLineWidth;
layout (location = 2) in vec4 aColour;

layout(std140) uniform render_state {
    mat4 projection;
    mat4 view;
};

out vec4 oColour;

void main() {
    gl_Position = projection * view * vec4(aVertex, 1.0);
    oColour = aColour;
}