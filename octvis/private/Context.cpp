//
// Date       : 11/01/2024
// Project    : octvis
// Author     : -Ry
//

#include "Context.h"

#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "Application.h"

#include <fstream>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

//############################################################################//
// | VERTEX & MODEL | Should be in their own file tbh
//############################################################################//

int octvis::RenderableModel::load_from_obj(const char* path_str) noexcept {

    fs::path path(path_str);

    if (!fs::exists(path)) {
        OCTVIS_WARN("File '{}' does not exist.", path_str);
        return -1;
    }

    // Filename excludes file extension
    model_name = path.stem().string();

    // Read File into a string
    using Iter = std::istreambuf_iterator<char>;
    std::fstream stream{ path };
    if (!stream || !stream.is_open()) {
        OCTVIS_ERROR("Filestream error occurred on file '{}'", path_str);
        return -1;
    }
    std::string content{ Iter{ stream }, Iter{} };

    // Init Variables & Process File
    vertices.resize(32);
    std::vector<glm::vec3> vertices_(32);
    std::vector<glm::vec3> normals_(32);
    std::vector<glm::vec2> tex_pos_(32);
    std::stringstream content_stream{ std::move(content) };

    std::string line{};
    line.reserve(256);
    while (content_stream >> line) {

        std::stringstream line_stream{ line };
        char id[2]{};
        Vertex vertex{};

        if (!(line_stream >> id)) continue;

        // Checks if the id is the provided
        constexpr auto is = [](char id[2], const char* val) -> bool {
            return (id[0] == val[0]) && (id[1] == val[1]);
        };

        // Parse Vertex
        if (is(id, "v ")) {
            glm::vec3 vec{};
            if (line_stream >> vec.x >> vec.y >> vec.z) {
                vertices_.push_back(vec);
            } else {
                OCTVIS_ERROR("OBJ Reading failed to parse vertex on input '{}'", line);
                return -1;
            }

            // Parse Texture Position
        } else if (is(id, "vt")) {
            glm::vec2 vt{};
            if (line_stream >> vt.x >> vt.y) {
                tex_pos_.push_back(vt);
            } else {
                OCTVIS_ERROR("OBJ Reading failed to parse vertex texture on input '{}'", line);
                return -1;
            }

            // Parse Vertex Normal
        } else if (is(id, "vn")) {
            glm::vec3 vec{};
            if (line_stream >> vec.x >> vec.y >> vec.z) {
                vertices_.push_back(vec);
            } else {
                OCTVIS_ERROR("OBJ Reading failed to parse vertex normal on input '{}'", line);
                return -1;
            }

            // Parse Face (We assume a triangle)
        } else if (is(id, "f ")) {
            char c; // this is used to eat separator chars
            glm::ivec3 vec[3]{};
            Vertex triangle[3]{};

            for (int i = 0; i < 3; ++i) {
                if (line_stream >> vec[i].x >> c >> vec[i].y >> c >> vec[i].z) {
                    vec[i] -= glm::ivec3{ 1 }; // Obj uses 1 Based Indexing
                    triangle[i] = Vertex{
                            .pos     = vertices_[vec[i].x],
                            .normal  = normals_[vec[i].y],
                            .tex_pos = tex_pos_[vec[i].z],
                            .colour  = glm::vec4{ 1 },
                    };
                } else {
                    OCTVIS_ERROR("OBJ Reading failed to parse vertex face on input '{}'", line);
                    return -1;
                }
            }

            vertices.push_back(triangle[0]);
            vertices.push_back(triangle[1]);
            vertices.push_back(triangle[2]);
        }

    }

    vertices.shrink_to_fit();
    upload_to_buffer();

    return 0;
}

void octvis::RenderableModel::load_triangle() noexcept {

    model_name = "Triangle";
    vertices.resize(3);

    // @off
    vertices[0] = Vertex{
            .pos     = glm::vec3{ -1.0F, -1.0F, 0.0F },
            .normal  = glm::vec3{ 0.0F, 0.0F, 1.0F   },
            .tex_pos = glm::vec2{ 0.0F, 0.0F         },
            .colour  = glm::vec4{ 1                  }
    };

    vertices[1] = Vertex{
            .pos     = glm::vec3{ 1.0F, -1.0F, 0.0F },
            .normal  = glm::vec3{ 0.0F,  0.0F, 1.0F },
            .tex_pos = glm::vec2{ 1.0F,  0.0F       },
            .colour  = glm::vec4{ 1                 }
    };

    vertices[2] = Vertex{
            .pos     = glm::vec3{ 0.0F, 1.0F, 0.0F },
            .normal  = glm::vec3{ 0.0F, 0.0F, 1.0F },
            .tex_pos = glm::vec2{ 0.5F, 1.0F       },
            .colour  = glm::vec4{ 1                }
    };
    // @on

    upload_to_buffer();
}

void octvis::RenderableModel::load_rect() noexcept {

}

void octvis::RenderableModel::load_cube() noexcept {

}

void octvis::RenderableModel::upload_to_buffer() noexcept {
    buffer.init<Vertex>(vertices.size(), vertices.data(), renderer::BufferUsage::STATIC);
}

void octvis::RenderableModel::attach_buffer_to_vao(
        octvis::renderer::VertexArrayObject& vao,
        size_t index
) noexcept {
    vao.attach_buffer(buffer)
       .add_interleaved_attributes<glm::vec3, glm::vec3, glm::vec2, glm::vec4>(index);
}

//############################################################################//
// | APPLICATION CONTEXT |
//############################################################################//

bool octvis::Context::start() noexcept {
    if (initialise_systems() != 0) {
        OCTVIS_ERROR("Failed to initialise context systems.");
        terminate_systems();
        return false;
    }
    start_update_loop();
    terminate_systems();
    return true;
}

void octvis::Context::add_app_impl(Application& app) noexcept {
    //@off
    app.m_Context    = this;
    app.m_Window     = &m_Window;
    app.m_Timing     = &m_Timing;
    app.m_Registry   = &m_Registry;
    if (m_IsApplicationRunning) {
        app.on_start();
    }
    //@on
}

//############################################################################//
// | INITIALISATION |
//############################################################################//

int octvis::Context::initialise_systems() noexcept {

    if (m_Window.handle != nullptr || m_Window.gl_context != nullptr) {
        OCTVIS_ERROR("Context has already been initialised.");
        return -1;
    }

    // Initialise SDL, GL Context, GLEW, and ImGui
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        OCTVIS_ERROR("Failed to initialise SDL; '{}'", SDL_GetError());
        return -1;
    }

    // GL Version & Profile
    SDL_GL_ResetAttributes();
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

    // Setup Graphics Context Data
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

    // Create Window
    m_Window.handle = SDL_CreateWindow(
            m_Window.title,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            800, 600,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
            | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (m_Window.handle == nullptr) {
        OCTVIS_TRACE("Failed to create SDL Window; Error: '{}'", SDL_GetError());
        return -1;
    }

    // Create Context & Bind
    m_Window.gl_context = SDL_GL_CreateContext(m_Window.handle);
    if (m_Window.gl_context == nullptr) {
        OCTVIS_TRACE("Failed to create GL Context; Error: '{}'", SDL_GetError());
        return -1;
    }

    SDL_GL_MakeCurrent(m_Window.handle, m_Window.gl_context);
    SDL_GL_SetSwapInterval(1);

    if (glewInit() != GLEW_OK) {
        OCTVIS_ERROR("Failed to initialise GLEW");
        return -1;
    }

    OCTVIS_TRACE(
            "Created Window & GL Context; {:p}, {:p}; GL Version '{}'",
            (void*) m_Window.handle,
            m_Window.gl_context,
            (const char*) glGetString(GL_VERSION)
    );

    // Initialise ImGui
    IMGUI_CHECKVERSION();
    OCTVIS_TRACE("Initialising ImGui");
    m_ImGuiContext = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();

    // Multiple Viewports
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Initialise Backend
    ImGui_ImplSDL2_InitForOpenGL(m_Window.handle, m_Window.gl_context);
    ImGui_ImplOpenGL3_Init("#version 450");

    return 0;
}

void octvis::Context::terminate_systems() noexcept {

    // These need to be deleted first
    m_Applications.clear();
    m_Models.clear();

    // Terminate ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(m_ImGuiContext);

    // Now we can destroy the context & window
    SDL_GL_DeleteContext(m_Window.gl_context);
    SDL_DestroyWindow(m_Window.handle);
    SDL_Quit();
}

//############################################################################//
// | PUBLIC METHODS |
//############################################################################//

octvis::Application* octvis::Context::find_app(const char* app_name) noexcept {

    for (int i = 0; i < m_Applications.size(); ++i) {
        auto* app = m_Applications[i].get();

        // Doing (app->m_AppName == app_name) should work but assumes string literals
        // point to the same object which the standard does not guarantee.
        if (strcmp(app->m_AppName, app_name) == 0) {
            return app;
        }
    }
    return nullptr;
}

void octvis::Context::clear(const glm::vec4& colour) {
    glClearColor(colour.r, colour.g, colour.b, colour.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void octvis::Context::swap_buffers() noexcept {
    SDL_GL_SwapWindow(m_Window.handle);
}

void octvis::Context::activate_context() noexcept {
    SDL_GL_MakeCurrent(m_Window.handle, m_Window.gl_context);
}

//############################################################################//
// | APPLICATION RUNTIME |
//############################################################################//

void octvis::Context::start_update_loop() noexcept {

    // Wordy
    using Clock = std::chrono::system_clock;
    using Instant = Clock::time_point;
    using FloatDuration = std::chrono::duration<float>;

    // Timing Metrics
    Instant start = Clock::now();
    Instant before = Clock::now();
    Instant after;
    float fixed_accumulator = 0.0F;

    // Start Each Application
    for (const auto& item : m_Applications) {
        item->on_start();
    }

    m_IsApplicationRunning = true;
    while (m_IsApplicationRunning) {

        after = Clock::now();

        // Total Runtime & Delta Time
        m_Timing.theta = FloatDuration{ after - start }.count();
        m_Timing.delta = FloatDuration{ after - before }.count();
        fixed_accumulator += m_Timing.delta;
        ++m_Timing.delta_ticks;

        // Handle Events
        process_events();

        // Prepare ImGui Frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Perform Fixed Update Ticks
        Instant update_instant = Clock::now();
        while (fixed_accumulator > m_Timing.fixed) {
            fixed_accumulator -= m_Timing.fixed;
            for (int j = 0; j < m_Applications.size(); ++j) {
                m_Applications[j]->on_fixed_update();
            }
            ++m_Timing.fixed_ticks;
        }
        m_Timing.fixed_update_total_time = FloatDuration{ Clock::now() - update_instant }.count();

        // Perform Updates
        update_instant = Clock::now();
        for (int i = 0; i < m_Applications.size(); ++i) {
            m_Applications[i]->on_update();
        }
        m_Timing.update_total_time = FloatDuration{ Clock::now() - update_instant }.count();

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update Viewports
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }

        SDL_GL_SwapWindow(m_Window.handle);

        before = after;
    }

    // Terminate the applications
    for (int i = 0; i < m_Applications.size(); ++i) {
        m_Applications[i]->on_finish();
    }

    OCTVIS_TRACE(
            "Application Terminating; Update Tick Count {}; Fixed Tick Count {}",
            m_Timing.delta_ticks, m_Timing.fixed_ticks
    );
}

//############################################################################//
// | EVENT PROCESSING |
//############################################################################//

void octvis::Context::process_events() noexcept {
    SDL_Event event{};
    m_InputSystem.reset();
    while (SDL_PollEvent(&event)) {

        ImGui_ImplSDL2_ProcessEvent(&event);

        // Standard Quit Condition
        if (event.type == SDL_QUIT) {
            stop();
            return;
        }

        // ImGui Specific Quit Condition
        if (event.type == SDL_WINDOWEVENT
            && event.window.event == SDL_WINDOWEVENT_CLOSE
            && event.window.windowID == SDL_GetWindowID(m_Window.handle)) {
            stop();
            return;
        }

        m_InputSystem.process_event(event);
    }
}