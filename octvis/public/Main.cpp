
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
        glm::vec3 initial_pos;
        glm::vec3 radius;
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
        constexpr size_t PADDING = 10;
        for (int i = 0; i < ENTITY_COUNT; ++i) {
            for (int j = 0; j < ENTITY_COUNT; ++j) {

                entt::entity entity = m_Registry->create();
                m_Registry->emplace<RenderableTag>(entity);
                TestEntityTag& tag = m_Registry->emplace<TestEntityTag>(entity);
                Renderable& r = m_Registry->emplace<Renderable>(entity);

                r.model_id = rand() % 3;
                r.colour = glm::vec4{ (25 + rand() % 75) / 100.0F };
                r.use_depth_test = true;
                r.use_face_culling = (r.model_id == 2);
                r.use_wireframe = (rand() % 2) == 1;

                Transform& t = m_Registry->emplace<Transform>(entity);
                srand(entt::to_integral(entity));
                t.position = glm::vec3{
                        (i + 1) * PADDING,
                        0,
                        (j + 1) * PADDING
                };
                t.scale = glm::vec3{ 1 + rand() % 3 };

                tag.initial_pos = t.position;
                tag.radius = glm::vec3{ 0, -5 + rand() % 10, 0 };
                tag.lerp = 0.0F;

            }
        }

        constexpr size_t LIGHT_COUNT = 8;
        for (int i = 0; i < LIGHT_COUNT; ++i) {
            entt::entity entity = m_Registry->create();

            // Renderable
            m_Registry->emplace<RenderableTag>(entity);
            Transform& t = m_Registry->emplace<Transform>(entity);
            t.scale = glm::vec3{0.5F};

            Renderable& r = m_Registry->emplace<Renderable>(entity);
            r.model_id = 1;
            r.colour = glm::vec4{ 1.0F };
            r.use_wireframe = false;
            r.use_depth_test = true;
            r.use_face_culling = false;

            // Light Source
            m_Registry->emplace<LightTag>(entity);
            PointLight& pl = m_Registry->emplace<PointLight>(entity);

            pl.diffuse = glm::vec3{(30 + rand() % 70) / 100.0F};
            pl.specular = glm::vec3{(30 + rand() % 70) / 100.0F};
            pl.shininess = float(1 << (4 + rand() % 6));
        }

    }

    virtual void on_update() noexcept override {

        ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

        const Camera& cam = m_Registry->get<Camera>(m_CameraEntity);

        auto group = m_Registry->group<TestEntityTag>(entt::get<Renderable, Transform>);

        using Clock = std::chrono::system_clock;
        using Instant = Clock::time_point;
        using FDuration = std::chrono::duration<float>;

        Instant start = Clock::now();

        std::for_each(
                std::execution::unseq,
                group.begin(),
                group.end(),
                [this](entt::entity e) {

                    Renderable& r = m_Registry->get<Renderable>(e);
                    Transform& t = m_Registry->get<Transform>(e);
                    TestEntityTag& tag = m_Registry->get<TestEntityTag>(e);

                    srand(entt::to_integral(e));
                    r.colour = glm::vec4{
                            (10 + rand() % 90) / 100.0F,
                            (10 + rand() % 90) / 100.0F,
                            (10 + rand() % 90) / 100.0F,
                            1.0F
                    };

                    tag.lerp += m_Timing->delta;
                    t.position = tag.initial_pos + tag.radius * std::sin(tag.lerp);
                    if (tag.lerp >= 3.1415F * 2.0F) {
                        tag.lerp = 0.0F;
                    }
                }
        );

        Instant end = Clock::now();

        if (ImGui::Begin("Main Debug")) {
            ImGui::Text(
                    "Entity Update Time %.4f (%llu)",
                    FDuration{ end - start }.count(),
                    group.size()
            );
        }
        ImGui::End();

        Camera& m_Camera = m_Registry->get<Camera>(m_CameraEntity);
        auto light_view = m_Registry->view<PointLight, Transform>();

        size_t count = 4;
        light_view.each(
                [&cam, &count, this](entt::entity e, PointLight& pl, Transform& trans) {
                    srand(entt::to_integral(e));

                    ++count;
                    float range = float(1 << count);

                    float r = -range + rand() % int(range * 2.0F);
                    float a = m_Timing->theta;
                    float b = a * float((75 + rand() % 125) / 100.0F);
                    glm::vec3 lerp{
                            r * std::sin(a) * std::cos(b),
                            std::abs(r),
                            r * std::cos(a),
                    };

                    pl.colour = glm::vec3{
                            ((50 + rand() % 50) / 100.0F ) * a,
                            ((50 + rand() % 50) / 100.0F ) * b,
                            ((50 + rand() % 50) / 100.0F ) * b
                    };
                    pl.colour = glm::clamp(pl.colour, 0.0F, 1.0F);

                    pl.position = cam.get_position() + lerp;
                    trans.position = pl.position;
                }
        );


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