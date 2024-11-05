#version 450

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

in vec4 oColour;
in vec3 oPos;
in vec3 oNormal;
in vec2 oUV;

out vec4 fColour;

//uniform sampler2D img;

vec3 get_phong_shading(in PointLight pl) {

    // Ambient
    vec3 ambient = pl.colour * pl.ambient;

    // Diffusion
    vec3 normal = normalize(oNormal);
    vec3 light_dir = normalize(pl.position - oPos);
    float diff = max(dot(normal, light_dir), 0.0);

    vec3 diffusion = diff * pl.diffuse * pl.colour;

    // Specular
    vec3 reflect_dir = reflect(-light_dir, normal);
    vec3 view_dir = normalize(cam_pos - oPos);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), pl.shininess);

    vec3 specular = (pl.colour * spec) * pl.specular;

    float distance = length(pl.position - oPos);
    float atten = 1.0 / (
        pl.attenuation.x + pl.attenuation.y * distance
        + pl.attenuation.z * (distance * distance)
    );

    return ambient + ((diffusion + specular) * atten);
}

void reinhard_mapping(inout vec3 lighting) {
    lighting = lighting / (1.0 + lighting);
}

void main() {

    // Assume no lighting
    if (active_lights == 0) {
        fColour = oColour;
        return;
    }

    // Compute Lighting
    vec3 lighting = vec3(0);
    for (int i = 0; i < active_lights; i++) {
        lighting += get_phong_shading(lights[i]);
    }
    reinhard_mapping(lighting);

    fColour = oColour * vec4(lighting, 1.0);
}