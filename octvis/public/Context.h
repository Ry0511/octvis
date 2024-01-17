//
// Date       : 11/01/2024
// Project    : octvis
// Author     : -Ry
//

#ifndef OCTVIS_CONTEXT_H
#define OCTVIS_CONTEXT_H

// @off
#define SDL_MAIN_HANDLED
#include "SDL.h"
// @on

#include "imgui.h"
#include "glm/glm.hpp"
#include "entt/entt.hpp"

#include "Renderer.h"
#include "InputSystem.h"

#include <vector>
#include <memory>

namespace octvis {

    //############################################################################//
    // | FORWARD DECLARATIONS |
    //############################################################################//

    class Application;

    struct Vertex {
        glm::vec3 pos, normal;
        glm::vec2 tex_pos;
        glm::vec4 colour;
    };

    struct RenderableModel {
        std::string model_name;
        std::vector<Vertex> vertices;
        renderer::Buffer buffer{ renderer::BufferType::ARRAY };

      public:
        int load_from_obj(const char* path) noexcept;
        void load_triangle() noexcept;
        void load_rect() noexcept;
        void load_cube() noexcept;

      public:
        void upload_to_buffer() noexcept;
        void attach_buffer_to_vao(renderer::VertexArrayObject& vao, size_t index) noexcept;
    };

    //############################################################################//
    // | STATEFUL CONTEXT INFO |
    //############################################################################//

    struct Window {
        //@off
        SDL_Window* handle       = nullptr;
        SDL_GLContext gl_context = nullptr;
        const char* title        = "Unknown";
        //@on
        int width, height;
        int x, y;
    };

    struct Timing {
        float theta = 0.0F;         // Context Runtime in Seconds
        float delta = 0.0F;         // Delta Time between Frames
        float fixed = 1.0F / 60.0F; // Fixed Time between Frames
        size_t delta_ticks = 0;     // Total number of delta ticks
        size_t fixed_ticks = 0;     // Total number of fixed ticks

        // Metrics
        float fixed_update_total_time = 0.0F;
        float update_total_time = 0.0F;
    };

    //############################################################################//
    // | APPLICATION CONTEXT |
    //############################################################################//

    class Context {

        // TODO: Find a better way to render vertex data
        // TODO: Setup and test ImGui

      private: // @off
        ImGuiContext*  m_ImGuiContext;
        Window         m_Window;
        Timing         m_Timing;
        InputSystem    m_InputSystem;
      // @on

      private:
        entt::registry m_Registry;
        std::vector<std::unique_ptr<Application>> m_Applications;
        std::unordered_map<std::string, RenderableModel> m_Models;
        bool m_IsApplicationRunning;

      public:
        Context() noexcept = default;
        ~Context() noexcept = default;

      public:
        Context(const Context&) = delete;
        Context(Context&&) = delete;
        Context& operator =(const Context&) = delete;
        Context& operator =(Context&&) = delete;

      public:
        template<class T, class... Args>
        inline void emplace_app(Args&& ... args) noexcept {
            static_assert(std::is_base_of_v<Application, T>, "T must derive Application");
            auto& app = m_Applications.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
            add_app_impl(*app);
        }

        inline RenderableModel& emplace_model(RenderableModel&& model) noexcept {
            auto it = m_Models.emplace(model.model_name, std::move(model));
            OCTVIS_ASSERT(it.second, "Failed to emplace '{}'", it.first->first);
            return it.first->second;
        }

      public:
        Application* find_app(const char* app_name) noexcept;
        void clear(const glm::vec4& colour = glm::vec4{ 0.15, 0.15, 0.15F, 1.0F });
        void swap_buffers() noexcept;
        void activate_context() noexcept;

      public:
        bool start() noexcept;
        inline void stop() noexcept {
            m_IsApplicationRunning = false;
        };

      private: // Bind values from context to the app
        void add_app_impl(Application& app) noexcept;

      private:
        int initialise_systems() noexcept;
        void terminate_systems() noexcept;
        void start_update_loop() noexcept;
        void process_events() noexcept;
    };

} // octvis

#endif
