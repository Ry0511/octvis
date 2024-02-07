#version 450

layout (location = 0) in vec3 aVertex;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in vec4 aColour;

// Instance Data
layout (location = 4) in vec4 iColour;
layout (location = 5) in mat4 iModel;
layout (location = 9) in mat3 iNormalMat;

struct PointLight {
    vec3 position;
    vec3 colour;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    vec3 attenuation;
};

layout(std140) uniform render_state {
    mat4 projection;
    mat4 view;
    vec3 cam_pos;
    int active_lights;
    PointLight lights[8];
};

out vec4 oColour;
out vec3 oPos;
out vec3 oNormal;
out vec2 oUV;

void main() {
    gl_Position = projection * view * iModel * vec4(aVertex, 1.0);
    oPos        = vec3(iModel * vec4(aVertex, 1.0));
    oColour     = iColour;
    oNormal     = iNormalMat * aNormal;
    oUV         = aUV;
}