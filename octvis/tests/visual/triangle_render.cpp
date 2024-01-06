//
// Date       : 01/01/2024
// Project    : octvis
// Author     : -Ry
//

#define OCTVIS_TESTS_INIT_VAR

#include "init.h"
#include "glm/gtc/matrix_transform.hpp"

using namespace octvis::renderer;
namespace ts = octvis_tests;

void* init_for_triangle();
void update_for_triangle(void*);
void draw_for_triangle(void*);
void cleanup(void*);

int main() {

    ts::init("Triangle Render Test");
    ts::set_functions(
            init_for_triangle,
            [](auto, auto) {},
            update_for_triangle,
            draw_for_triangle,
            cleanup
    );
    ts::start();

}

//############################################################################//
// | INIT |
//############################################################################//

struct Config {
    Buffer vertex_buffer{ BufferType::ARRAY };
    Buffer uniform_buffer{ BufferType::UNIFORM };
    ShaderProgram program;
    VertexArrayObject vao;
};

struct Vertex {
    glm::vec3 pos, colour;
};

struct RenderConfig {
    glm::mat4 projection;
    glm::mat4 camera;
    glm::mat4 model;
};

void* init_for_triangle() {
    Config* cfg = new Config{};

    Vertex vertices[]{
            { glm::vec3{ 0.0F, 1.0F, 0.0F },   glm::vec3{ 1.0F, 0.0F, 1.0F } },
            { glm::vec3{ -1.0F, -1.0F, 0.0F }, glm::vec3{ 0.0F, 1.0F, 0.0F } },
            { glm::vec3{ 1.0F, -1.0F, 0.0F },  glm::vec3{ 0.0F, 0.0F, 1.0F } }
    };
    cfg->vertex_buffer.init(3, vertices, BufferUsage::STATIC);

    RenderConfig render_config{
            glm::perspective(90.0F, 16.0F / 9.0F, 0.1F, 100.0F),
            glm::lookAt(glm::vec3{ 0, 0, -2.5F }, glm::vec3{ 0, 0, 0 }, glm::vec3{ 0, 1, 0 }),
            glm::scale(glm::mat4{ 1 }, glm::vec3{ 1.0F, 1.0F, 1.0F })
    };
    cfg->uniform_buffer.init(1, &render_config, BufferUsage::DYNAMIC);

    {
        ShaderProgram& prog = cfg->program;
        prog.init();
        Shader vertex_shader{ ShaderType::VERTEX };
        vertex_shader.load_from_path("resources/VertexShader.glsl");

        Shader frag_shader{ ShaderType::FRAGMENT };
        frag_shader.load_from_path("resources/FragmentShader.glsl");

        vertex_shader.attach(prog);
        frag_shader.attach(prog);

        cfg->program.link();
    }

    VertexArrayObject& vao = cfg->vao;
    vao.init();
    vao.attach_buffer(cfg->vertex_buffer)
       .add_interleaved_attributes<glm::vec3, glm::vec3>(0);

    return cfg;
};

void update_for_triangle(void* ptr) {
    Config& cfg = *static_cast<Config*>(ptr);

    RenderConfig& render = *static_cast<RenderConfig*>(cfg.uniform_buffer.create_mapping());

    static glm::vec3 skew_r{ 1.0F, 1.0F, 1.0F };

    if (ts::theta > 3.1415F * 0.5F) {
        ts::theta = 0.0F;

        constexpr auto random_float = []() -> float {
            return (rand() % 100) * 0.01F;
        };
        skew_r = glm::vec3{ random_float(), random_float(), random_float() };
    }

    render.model = glm::rotate(glm::mat4{ 1 }, 360.0F * std::sin(ts::theta), skew_r);

    cfg.uniform_buffer.release_mapping();
}

void draw_for_triangle(void* data) {
    Config& cfg = *static_cast<Config*>(data);
    cfg.program.activate();
    cfg.program.set_ubo(cfg.uniform_buffer, 0, "render_config");
    glEnable(GL_MULTISAMPLE);
    glLineWidth(2.0F);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisable(GL_MULTISAMPLE);
    cfg.program.deactivate();
}

void cleanup(void* ptr) {
    delete static_cast<Config*>(ptr);
}