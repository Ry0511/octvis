
// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

//
// TODO: Integrate the RawImage with stb_image with a bit more cohesiveness and abstractness.
//

#include "Logging.h"

#include "Renderer.h"
#include "glm/gtc/matrix_transform.hpp"

//
// Plan for branch merge.
//

using namespace octvis::renderer;

#define SDL_MAIN_HANDLED

#include "SDL.h"

void* initialise();
void update(void*, float);
void render(void*);
void cleanup(void*);

int main(int, char**) {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        OCTVIS_ERROR("Error: {}", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags) (
            SDL_WINDOW_OPENGL
            | SDL_WINDOW_RESIZABLE
            | SDL_WINDOW_ALLOW_HIGHDPI
    );
    SDL_Window* window = SDL_CreateWindow(
            "Octvis",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            800,
            600,
            window_flags
    );
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    if (glewInit() != GLEW_OK) {
        OCTVIS_ERROR("Failed to initialise GLEW.");
        return -1;
    }

    void* state = initialise();

    using Clock = std::chrono::high_resolution_clock;
    using Instant = Clock::time_point;

    Instant start;
    Instant end;
    float delta = 0.0F;

    constexpr size_t frame_timing_size = 1024;
    std::vector<float> frame_timing{};
    frame_timing.reserve(frame_timing_size);
    frame_timing.resize(frame_timing_size, 0.0F);
    size_t index = 0;
    double fps = -1.0F;

    bool is_running = true;
    while (is_running) {

        if (index == frame_timing_size - 1) {
            index = 0;

            double sum = 0.0;
            for (float& t : frame_timing) {
                sum += t;
                t = 0.0F;
            }
            fps = (double{ 1.0F } * double{ frame_timing_size }) / sum;
            OCTVIS_TRACE("Average FPS over {} updates {:.3f}", frame_timing_size, fps);
            frame_timing.resize(frame_timing_size, 0.0F);
        }

        start = Clock::now();

        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                is_running = false;
            }
        }

        int width, height;
        SDL_GetWindowSize(window, &width, &height);

        glViewport(0, 0, width, height);
        glClearColor(0.2F, 0.2F, 0.2F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glLineWidth(1.0F);

        std::string title = std::format("Octvis :: {:.2f}, {}, {:.3f}ms", fps, (size_t) (1.0F / delta), delta);
        SDL_SetWindowTitle(window, title.c_str());

        update(state, delta);
        render(state);

        SDL_GL_SwapWindow(window);

        end = Clock::now();
        delta = std::chrono::duration<float>(end - start).count();

        frame_timing[index] = delta;
        ++index;
    }

    cleanup(state);

    return 0;
}

//############################################################################//
// |  |
//############################################################################//

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 colour;
    glm::vec2 tex_coord;
};

struct RenderState {
    Buffer vertex_buffer{ BufferType::ARRAY };
    Buffer uniform_buffer{ BufferType::UNIFORM };
    ShaderProgram shader_program{};
    VertexArrayObject vao{};
    Texture2D texture{};
};

struct UniformState {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
};

void* initialise() {

    RenderState* state = new RenderState{};

    // @off
    Vertex vertices[]{
            { glm::vec3{  0.00F,  1.00F,  0.00F }, glm::vec3{ 0.0F, 0.0F, 0.0F }, glm::vec3{ 0.0F, 0.0F, 1.0F }, glm::vec2{ 0.5F, 1.0F } },
            { glm::vec3{ -1.00F, -1.00F,  0.00F }, glm::vec3{ 0.0F, 0.0F, 0.0F }, glm::vec3{ 0.0F, 1.0F, 0.0F }, glm::vec2{ 0.0F, 0.0F } },
            { glm::vec3{  1.00F, -1.00F,  0.00F }, glm::vec3{ 0.0F, 0.0F, 0.0F }, glm::vec3{ 1.0F, 0.0F, 0.0F }, glm::vec2{ 1.0F, 0.0F } },
    };
    // @on

    state->vertex_buffer.init(3, vertices, BufferUsage::STATIC);

    UniformState ubo{
            glm::perspective(90.0F, 16.0F / 9.0F, 0.1F, 100.0F),
            glm::lookAt(glm::vec3{ 0, 0, -1 }, glm::vec3{ 0, 0, 0 }, glm::vec3{ 0, 1, 0 }),
            glm::scale(glm::mat4{ 1 }, glm::vec3{ 0.8F, 0.8F, 0.8F })
    };
    state->uniform_buffer.init(1, &ubo, BufferUsage::DYNAMIC);

    state->shader_program.init();
    {
        Shader vertex{ ShaderType::VERTEX };
        vertex.load_from_path(
                "G:\\University\\Year 4\\CSP400\\Project\\octvis\\octvis\\resources\\VertexShader_UBO.glsl"
        );
        state->shader_program.attach_shader(vertex);

        Shader fragment{ ShaderType::FRAGMENT };
        fragment.load_from_path(
                "G:\\University\\Year 4\\CSP400\\Project\\octvis\\octvis\\resources\\FragmentShader.glsl"
        );
        state->shader_program.attach_shader(fragment);

        state->shader_program.link();
    }

    state->vao.init();
    state->vao.bind();
    state->vao.attach_buffer(state->vertex_buffer)
         .add_interleaved_attributes<glm::vec3, glm::vec3, glm::vec3, glm::vec2>(0);
    state->vao.unbind();

    // Load Image
    RawImage img{};
    img.format = ColourFormat::RGBA;
    img.pixel_type = PixelType::UBYTE;
    img.load_from_path("G:\\University\\Year 4\\CSP400\\Project\\octvis\\octvis\\resources\\Untitled.png");

    // Init Texture
    state->texture.init(img);

    return state;
}

void update(void* ptr, float delta) {

    RenderState* state = static_cast<RenderState*>(ptr);

    UniformState* uniforms = static_cast<UniformState*>(state->uniform_buffer.create_mapping());

    static float theta = 0.0F;
    theta += delta * 2.0F;
    if (theta >= 3.1415f * 2.0F) theta = 0.0F;

    float radius = 2.0F;

    glm::vec3 pos{
            radius * std::cos(theta),
            radius * std::sin(theta) + std::cos(theta),
            radius * std::sin(theta),
    };

    uniforms->view = glm::lookAt(pos, glm::vec3{ 0, 0, 0 }, glm::vec3{ 0, 1, 0 });

    state->uniform_buffer.release_mapping();
}

void render(void* ptr) {

    RenderState* state = static_cast<RenderState*>(ptr);

    state->vao.bind();
    state->shader_program.activate();
    state->shader_program.set_ubo(state->uniform_buffer, 0, "render_state");
    state->shader_program.set_texture(state->texture, 0, "img");
    glDrawArrays(GL_TRIANGLES, 0, 3);

    state->shader_program.deactivate();
    state->vao.unbind();

}

void cleanup(void* ptr) {
    delete static_cast<RenderState*>(ptr);
}