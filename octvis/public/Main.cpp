
#include "Logging.h"
#include "Context.h"
#include "Application.h"
#include "RenderApplication.h"

#include <algorithm>
#include <execution>
#include <chrono>

using namespace octvis;
using namespace octvis::renderer;

class TestApp : public Application {

  private:
    struct TestEntityTag {
        float theta;
        float lerp;
    };

  private:
    entt::entity m_CameraEntity;

  public:
    TestApp() : Application("Test App") {}

  public:
    virtual void on_start() noexcept override {
        OCTVIS_TRACE("Test App Starting!");

        m_CameraEntity = m_Registry->create();
        m_Registry->emplace<CameraTag>(m_CameraEntity);
        Camera& camera = m_Registry->emplace<Camera>(m_CameraEntity);

        camera.set_projection(90.0F, 0.1F, 1000.0F, 800.0F / 600.0F);
        camera.set_position(glm::vec3{ 0, 0, -5 });
        camera.look_at(glm::vec3{ 0, 0, 0 });

        constexpr size_t ENTITY_COUNT = 16;
        constexpr size_t PADDING = 32;
        for (int i = 0; i < ENTITY_COUNT; ++i) {
            for (int j = 0; j < ENTITY_COUNT; ++j) {

                entt::entity entity = m_Registry->create();
                m_Registry->emplace<RenderableTag>(entity);
                m_Registry->emplace<TestEntityTag>(entity);
                Renderable& r = m_Registry->emplace<Renderable>(entity);

                r.model_id = rand() % 3;
                r.colour = glm::vec4{(25 + rand() % 75) / 100.0F};
                r.use_depth_test = true;
                r.use_face_culling = false;
                r.use_wireframe = (rand() % 2) == 1;

                Transform& t = m_Registry->emplace<Transform>(entity);
                srand(entt::to_integral(entity));
                t.position = glm::vec3{
                        (i + 1) * PADDING,
                        0,
                        (j + 1) * PADDING
                };
                t.scale = glm::vec3{ 4 + rand() % 12 };

            }
        }

        constexpr size_t LIGHT_COUNT = 8;
        for (int i = 0; i < LIGHT_COUNT; ++i) {
            entt::entity entity = m_Registry->create();

            // Renderable
            m_Registry->emplace<RenderableTag>(entity);
            m_Registry->emplace<Transform>(entity);
            Renderable& r = m_Registry->emplace<Renderable>(entity);
            r.model_id = 1;
            r.colour = glm::vec4{ 1.0F };
            r.use_wireframe = true;
            r.use_depth_test = true;
            r.use_face_culling = false;

            // Light Source
            m_Registry->emplace<LightTag>(entity);
            m_Registry->emplace<PointLight>(entity);
        }

    }

    virtual void on_update() noexcept override {

        ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

        const Camera& cam = m_Registry->get<Camera>(m_CameraEntity);

        auto group = m_Registry->group<TestEntityTag>(entt::get<Renderable, Transform>);

        // Light Position
        auto view = m_Registry->view<LightTag, PointLight, Transform>();

        view.each(
                [this, &cam](entt::entity e, PointLight& light, Transform& trans) {
                    srand(entt::to_integral(e));

                    float a, b, c;
                    a = m_Timing->theta;
                    b = m_Timing->theta * ((80 + rand() % 145) / 100.0F);
                    c = b * 0.73;

                    glm::vec3 r{-20 + rand() % 40, -10 + rand() % 20, -20 + rand() % 40};
                    r += 5;

                    light.position = cam.get_position() + glm::vec3{
                        r.x * std::sin(a) * std::cos(b),
                        r.y * std::sin(a),
                        r.z * std::cos(a) * std::sin(b),
                    };

                    light.colour = glm::vec3{
                            (30 + rand() % 70) / 100.0F,
                            (30 + rand() % 70) / 100.0F,
                            (30 + rand() % 70) / 100.0F,
                    };

                    light.diffuse = glm::vec3{0.0F};
                    light.specular = glm::vec3{1.5F};
                    light.shininess = std::max((1 << (5 + rand() % 5)) * std::sin(b), 8.0F);

                    trans.position = light.position;
                    trans.scale = glm::vec3{ 0.001F };
                    trans.rotation = glm::vec3{ get_rotation_to(cam.get_position(), trans.position), 0.0F };

                }
        );

        std::for_each(
                std::execution::seq,
                group.begin(),
                group.end(),
                [this, &cam](entt::entity e) {

                    Renderable& r = m_Registry->get<Renderable>(e);
                    Transform& t = m_Registry->get<Transform>(e);

                    srand(entt::to_integral(e));
                    r.colour = glm::vec4{
                            (10 + rand() % 90) / 100.0F,
                            (10 + rand() % 90) / 100.0F,
                            (10 + rand() % 90) / 100.0F,
                            1.0F
                    };
                    t.rotation = glm::vec3{ get_rotation_to(t.position, cam.get_position()), 0.0F };
                }
        );

        Camera& m_Camera = m_Registry->get<Camera>(m_CameraEntity);

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
        float vel = 20.0F * m_Timing->delta;

        if (InputSystem::is_key_pressed(SDLK_w)) m_Camera.translate_forward(vel);
        if (InputSystem::is_key_pressed(SDLK_s)) m_Camera.translate_forward(-vel);
        if (InputSystem::is_key_pressed(SDLK_a)) m_Camera.translate_horizontal(-vel);
        if (InputSystem::is_key_pressed(SDLK_d)) m_Camera.translate_horizontal(vel);

        glm::vec3 pos = m_Camera.get_position();
        m_Camera.set_position(pos);

        // Capture Mouse
        static bool is_relative_mode = false;
        if (InputSystem::is_key_released(SDLK_ESCAPE)) {
            is_relative_mode = !is_relative_mode;
            SDL_SetRelativeMouseMode(is_relative_mode ? SDL_TRUE : SDL_FALSE);
        }

        // Looking Around
        if (is_relative_mode) {
            glm::vec2 mvel = InputSystem::get_mouse_vel() * m_Timing->delta * 2.0F;
            m_Camera.look(mvel);
        }

        if (InputSystem::is_key_pressed(SDLK_LEFT)) m_Camera.look_horizontal(-m_Timing->fixed);
        if (InputSystem::is_key_pressed(SDLK_RIGHT)) m_Camera.look_horizontal(m_Timing->fixed);
        if (InputSystem::is_key_pressed(SDLK_UP)) m_Camera.look_vertical(m_Timing->fixed);
        if (InputSystem::is_key_pressed(SDLK_DOWN)) m_Camera.look_vertical(-m_Timing->fixed);

        static bool is_movement_xz = false;
        if (InputSystem::is_key_released(SDLK_1)) {
            is_movement_xz = !is_movement_xz;
            m_Camera.set_move_xz(is_movement_xz);
        }

        if (InputSystem::is_key_released(SDLK_r)) m_Camera.look_at(glm::vec3{ 0 });

        if (ImGui::Begin("Debug")) {
            ImGui::Text("W => %s", InputSystem::is_key_pressed(SDLK_w) ? "Pressed" : "Released");
            ImGui::Text("A => %s", InputSystem::is_key_pressed(SDLK_a) ? "Pressed" : "Released");
            ImGui::Text("S => %s", InputSystem::is_key_pressed(SDLK_s) ? "Pressed" : "Released");
            ImGui::Text("D => %s", InputSystem::is_key_pressed(SDLK_d) ? "Pressed" : "Released");

            auto mvel = InputSystem::get_mouse_vel();
            ImGui::Text("MVel => ( %.4f, %.4f )", mvel.x, mvel.y);

            glm::vec3 position = m_Camera.get_position();
            glm::vec3 rotation = m_Camera.get_rotate();
            ImGui::Text("Position ( %.3f, %.3f, %.3f )", pos.x, pos.y, pos.z);
            ImGui::Text("YPR      ( %.3f, %.3f, %.3f )", rotation.x, rotation.y, rotation.z);
        }
        ImGui::End();

        if (ImGui::Begin("Application Timings")) {
            ImGui::Text("Framerate          %0f", 1.0F / m_Timing->delta);
            ImGui::Text("Theta              %2f", m_Timing->theta);
            ImGui::Text("Delta              %4f", m_Timing->delta);
            ImGui::Text("Fixed              %4f", m_Timing->fixed);
            ImGui::Text("Delta Ticks        %llu", m_Timing->delta_ticks);
            ImGui::Text("Fixed Ticks        %llu", m_Timing->fixed_ticks);
            ImGui::Text("Fixed Update Theta %4f", m_Timing->fixed_update_total_time);
            ImGui::Text("Delta Update Theta %4f", m_Timing->update_total_time);
        }
        ImGui::End();

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
    ctx.emplace_app<RenderApplication>();
    if (!ctx.start()) return -1;
    return 0;
}