//
// Date       : 17/01/2024
// Project    : octvis
// Author     : -Ry
//

#ifndef OCTVIS_RENDERAPPLICATION_H
#define OCTVIS_RENDERAPPLICATION_H

#include "Application.h"
#include "Renderer.h"

#include <unordered_map>
#include <functional>

namespace octvis {

    //############################################################################//
    // | PULL IN RENDERER CLASSES |
    //############################################################################//

    using renderer::Buffer;
    using renderer::DynamicBuffer;
    using renderer::BufferType;
    using renderer::BufferUsage;
    using renderer::Shader;
    using renderer::ShaderType;
    using renderer::ShaderProgram;
    using renderer::VertexArrayObject;
    using renderer::Texture2D;

    //############################################################################//
    // | COMPONENTS |
    //############################################################################//

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 tex_pos;
        glm::vec4 colour;
    };

    struct RenderableTag {};
    struct CameraTag {};

    struct alignas(32) Renderable {
      public:
        int model_id;
        int texture_id;
        glm::vec4 colour;

      public:
        bool use_depth_test = true;
        bool use_face_culling = true;
        bool use_wireframe = false;

        size_t get_hash() const noexcept {
            size_t hash = model_id;
            hash = (hash << 1) | static_cast<int>(use_depth_test);
            hash = (hash << 1) | static_cast<int>(use_face_culling);
            hash = (hash << 1) | static_cast<int>(use_wireframe);
            return hash;
        }

        size_t get_state_hash() const noexcept {
            return (static_cast<int>(use_depth_test) << 0)
                   | (static_cast<int>(use_face_culling) << 1)
                   | (static_cast<int>(use_wireframe) << 2);
        }
    };

    struct LineRenderable {
        bool enabled{true};
        std::vector<glm::vec3> vertices{};
        float line_width{2.0F};
        glm::vec4 colour{1.0F, 0.0F, 1.0F, 1.0F};
    };

    struct RenderState {
        static constexpr size_t LIGHT_COUNT = 8;

        alignas(64) glm::mat4 projection;
        alignas(64) glm::mat4 view;
        alignas(16) glm::vec3 cam_pos;
        alignas(4) int active_lights;
        alignas(16) PointLight lights[LIGHT_COUNT]{};
    };

    std::vector<Vertex> create_wireframe(const std::vector<Vertex>& vertices);

    //############################################################################//
    // | RENDER APPLICATION |
    //############################################################################//

    class RenderApplication : public Application {

      private: // @off
        struct ModelImpl {
            std::vector<Vertex> vertices{};
            size_t vertex_count = 0; // Vertex Count
            size_t begin        = 0; // Start Index
        };

        struct InstanceData {
            glm::vec4 colour;
            glm::mat4 model;
            glm::mat3 normal_matrix;
        };

        struct MultiDrawCommand {
            unsigned int count;
            unsigned int instance_count;
            unsigned int first;
            unsigned int base_instance = 0;
        };

        struct LineRenderState {
            alignas(64) glm::mat4 proj;
            alignas(64) glm::mat4 view;
        };

        struct LineState {
            float line_width{2.0F};
            glm::vec4 colour{1.0F, 1.0F, 1.0F, 1.0F};
        };

        struct LineRenderContextTag {};

        struct LineRenderContext {
            std::shared_ptr<ShaderProgram> shader;
            std::shared_ptr<Buffer> state;
            std::shared_ptr<DynamicBuffer<glm::vec3>> lines;
            std::shared_ptr<DynamicBuffer<LineState>> line_states;
            std::shared_ptr<DynamicBuffer<MultiDrawCommand>> commands;
            std::shared_ptr<VertexArrayObject> vao;

            inline LineRenderContext()
            : shader(std::make_shared<ShaderProgram>()),
              state(std::make_shared<Buffer>(BufferType::UNIFORM)),
              lines(std::make_shared<DynamicBuffer<glm::vec3>>(BufferType::ARRAY)),
              line_states(std::make_shared<DynamicBuffer<LineState>>(BufferType::ARRAY)),
              commands(std::make_shared<DynamicBuffer<MultiDrawCommand>>(BufferType::DRAW_INDIRECT)),
              vao(std::make_shared<VertexArrayObject>()) {
            }

            ~LineRenderContext() {
                OCTVIS_TRACE("DESTROYING LINE RENDER CONTEXT!");
            }
        };
      // @on

      private: // @off
        std::vector<Texture2D> m_Textures;
        std::vector<ModelImpl> m_Models;

        ShaderProgram                m_ShaderProgram;
        DynamicBuffer<Vertex>*       m_ModelBuffer;
        DynamicBuffer<InstanceData>* m_InstanceBuffer;
        Buffer*                      m_UniformBuffer;
        Buffer*                      m_CommandBuffer;
        VertexArrayObject            m_VAO;

        LineRenderContext* m_LineContext;
      // @on

      private:
        static RenderApplication* s_App;

      public:
        inline static RenderApplication& get() {
            OCTVIS_ASSERT(s_App != nullptr, "Render application has not been initialised.");
            return *s_App;
        }

      public:
        RenderApplication() noexcept;

        ~RenderApplication() noexcept;

      public:
        virtual void on_start() noexcept override;

        virtual void on_update() noexcept override;

      public:
        int add_model(const char* path);

        int add_model(std::vector<Vertex>&& vertices);

      private:
        void debug_init_triangle();

        void debug_init_rect();

        void debug_init_cube();

        void debug_init_sphere();

      public:
        void on_renderable_created(entt::basic_registry<>&, entt::entity) noexcept;

        void on_renderable_updated(entt::basic_registry<>&, entt::entity) noexcept;

        void on_renderable_destroyed(entt::basic_registry<>&, entt::entity) noexcept;

      private:
        void update_render_state() noexcept;

        void render_instance_data(
                const Renderable&,
                const std::vector<MultiDrawCommand>&,
                const std::vector<InstanceData>&
        ) noexcept;

      private:
        void render_lines() noexcept;

    };

} // octvis

#endif
