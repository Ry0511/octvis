
#include "Logging.h"
#include "Context.h"
#include "Application.h"

using namespace octvis;
using namespace octvis::renderer;

class TestApp : public Application {

  public:
    struct RenderState {
        glm::mat4 projection, view, model;
    };

    struct RenderContext {
        ShaderProgram m_Program;
        RenderState m_RenderState;
        Buffer m_RenderStateBuffer{ BufferType::UNIFORM };
        VertexArrayObject m_Vao;
    };

  private:
    Camera m_Camera;
    std::unique_ptr<RenderContext> m_RenderContext{ nullptr };

  public:
    TestApp() : Application("Test App") {}

  public:
    virtual void on_start() noexcept override {
        OCTVIS_TRACE("Test App Starting!");

        m_RenderContext = std::make_unique<RenderContext>();

        m_RenderContext->m_Program.init();
        {
            Shader vertex{ ShaderType::VERTEX };
            vertex.load_from_path("resources/VertexShader_UBO.glsl");
            m_RenderContext->m_Program.attach_shader(vertex);

            Shader fragment{ ShaderType::FRAGMENT };
            fragment.load_from_path("resources/FragmentShader.glsl");
            m_RenderContext->m_Program.attach_shader(fragment);

            m_RenderContext->m_Program.link();
        }

        m_Camera.set_position(glm::vec3{ 0, 0, -5 });
        m_Camera.look_at(glm::vec3{ 0, 0, 0 });
        m_RenderContext->m_RenderState.projection = m_Camera.get_projection();
        m_RenderContext->m_RenderState.view = m_Camera.get_view_matrix();
        m_RenderContext->m_RenderState.model = glm::scale(glm::mat4{ 1 }, glm::vec3{ 0.8F, 0.8F, 0.8F });
        m_RenderContext->m_RenderStateBuffer.init<RenderState>(
                1, &m_RenderContext->m_RenderState, BufferUsage::DYNAMIC
        );

        RenderableModel model_;
        model_.load_triangle();
        // RenderableModel& model = m_Context->emplace_model(std::move(model_));

        m_RenderContext->m_Vao.init();
        model_.attach_buffer_to_vao(m_RenderContext->m_Vao, 0);
        m_RenderContext->m_Vao.unbind();

    }

    virtual void on_update() noexcept override {

        std::string title = std::format(
                "{} :: {:.3f}, {:.5f}, {}, {:.5f}, {}; ( {:2f}, {:2f}, {:2f} ), ( {:2f}, {:2f}, {:2f} )",
                m_AppName,
                m_Timing->theta,
                m_Timing->delta,
                m_Timing->delta_ticks,
                m_Timing->fixed,
                m_Timing->fixed_ticks,

                m_Camera.get_rotate().x,
                m_Camera.get_rotate().y,
                m_Camera.get_rotate().z,

                m_Camera.get_position().x,
                m_Camera.get_position().y,
                m_Camera.get_position().z
        );
        SDL_SetWindowTitle(m_Window->handle, title.c_str());

        int width, height;
        SDL_GetWindowSize(m_Window->handle, &width, &height);
        glViewport(0, 0, width, height);
        m_Context->clear();

        // Moving Around
        float vel = 5.0F * m_Timing->delta;
        if (InputSystem::is_key_pressed(SDLK_w)) m_Camera.translate_forward(vel);
        if (InputSystem::is_key_pressed(SDLK_s)) m_Camera.translate_forward(-vel);
        if (InputSystem::is_key_pressed(SDLK_a)) m_Camera.translate_horizontal(-vel);
        if (InputSystem::is_key_pressed(SDLK_d)) m_Camera.translate_horizontal(vel);

        glm::vec3 pos = m_Camera.get_position();
        pos.y = 0.0F;
        m_Camera.set_position(pos);

        // Capture Mouse
        static bool is_relative_mode = false;
        if (InputSystem::is_key_released(SDLK_ESCAPE)) {
            is_relative_mode = !is_relative_mode;
            SDL_SetRelativeMouseMode(is_relative_mode ? SDL_TRUE : SDL_FALSE);
        }

        // Looking Around
        if (is_relative_mode) {
            glm::vec2 mvel = InputSystem::get_mouse_vel() * 3.0F * m_Timing->delta;
            m_Camera.look(mvel);
        }

        static bool is_movement_xz = false;
        if (InputSystem::is_key_released(SDLK_1)) {
            is_movement_xz = !is_movement_xz;
            m_Camera.set_move_xz(is_movement_xz);
        }

        if (InputSystem::is_key_released(SDLK_r)) m_Camera.look_at(glm::vec3{ 0 });

        ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
        if (ImGui::Begin("Application Timings")) {
            ImGui::Text("Theta              %4f", m_Timing->theta);
            ImGui::Text("Delta              %4f", m_Timing->delta);
            ImGui::Text("Fixed              %4f", m_Timing->fixed);
            ImGui::Text("Delta Ticks        %llu", m_Timing->delta_ticks);
            ImGui::Text("Fixed Ticks        %llu", m_Timing->fixed_ticks);
            ImGui::Text("Fixed Update Theta %4f", m_Timing->fixed_update_total_time);
            ImGui::Text("Delta Update Theta %4f", m_Timing->update_total_time);
        }
        ImGui::End();

        RenderState* state = static_cast<RenderState*>(m_RenderContext->m_RenderStateBuffer.create_mapping());
        state->view = m_Camera.get_view_matrix();
        m_RenderContext->m_RenderStateBuffer.release_mapping();

        m_RenderContext->m_Vao.bind();
        m_RenderContext->m_Program.activate();
        m_RenderContext->m_Program.set_ubo(m_RenderContext->m_RenderStateBuffer, 0, "render_state");
        glDrawArrays(GL_TRIANGLES, 0, 3);
        m_RenderContext->m_Vao.unbind();
        m_RenderContext->m_Program.deactivate();
    }

    virtual void on_fixed_update() noexcept override {
    }

    virtual void on_finish() noexcept override {
        OCTVIS_TRACE("Test App Finished!");
    }

};

int main(int, char**) {
    Context ctx{};
    ctx.emplace_app<TestApp>();
    if (!ctx.start()) return -1;
    return 0;
}