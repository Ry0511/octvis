
#include "Logging.h"
#include "Context.h"
#include "Application.h"
#include "RenderApplication.h"
#include "OctreeVisualisationApp.h"
#include "PhysicsSystem.h"

#include <algorithm>
#include <execution>
#include <chrono>

using namespace octvis;
using namespace octvis::renderer;

class TestApp : public Application {

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
        camera.set_position(glm::vec3{0, 0, -5});
        camera.look_at(glm::vec3{0, 0, 0});

        // Floor
        entt::entity floor = m_Registry->create();
        m_Registry->emplace<RenderableTag>(floor);
        m_Registry->emplace<ModelMatrix>(floor);

        Renderable& r = m_Registry->emplace<Renderable>(floor);
        r.model_id = 2;
        r.colour = glm::vec4{0.77F, 0.77F, 0.77F, 1.0F};
        r.use_depth_test = true;
        r.use_face_culling = true;
        r.use_wireframe = false;

        Transform& t = m_Registry->emplace<Transform>(floor);
        t.position = glm::vec3{0.0F, -1.0F, 0.0F};
        t.scale = glm::vec3{1000.0F, 1.0F, 1000.0F};

        for (int i = 0; i < RenderState::LIGHT_COUNT; ++i) {
            entt::entity e = m_Registry->create();
            m_Registry->emplace<LightTag>(e);
            PointLight& light = m_Registry->emplace<PointLight>(e);
            light.position = glm::vec3{-5 + rand() % 5, 10.0F, -5 + rand() % 5};
            light.colour = glm::vec3{1.0F, 1.0F, 1.0F};
            light.diffuse *= 1.35F;
        }


        entt::entity player_collider = m_Registry->create();

        m_Registry->emplace<CameraTag>(player_collider);
        m_Registry->emplace<ColliderTag>(player_collider);
        m_Registry->emplace<RenderableTag>(player_collider);
        m_Registry->emplace<Transform>(player_collider);
        m_Registry->emplace<LineRenderable>(player_collider);
        m_Registry->emplace<BoxCollider>(player_collider);

        RigidBody& rb = m_Registry->emplace<RigidBody>(player_collider);
        rb.mass = 0;
        rb.friction = 0;

        CollisionTracker& tracker = m_Registry->emplace<CollisionTracker>(player_collider);
        tracker.callback = [player_collider, this](entt::entity e1, size_t num_tests, size_t num_collisions) {

            Transform& player_pos = m_Registry->get<Transform>(player_collider);
            Transform& collidee_pos = m_Registry->get<Transform>(e1);
            LineRenderable& lines = m_Registry->get<LineRenderable>(player_collider);
            lines.colour = glm::vec4{0.33F, 0.8F, 1.0F, 1.0F};

            if (num_tests == 1) lines.vertices.clear();
            lines.vertices.push_back(player_pos.position - glm::vec3{0.0F, 1.0F, 0.0F});
            lines.vertices.push_back(collidee_pos.position);

        };


        m_Timing->fixed = 1.0F / 60.0F;
    }

    virtual void on_update() noexcept override {

        ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

        Camera& cam = m_Registry->get<Camera>(m_CameraEntity);

        {
            m_Registry->view<CameraTag, LineRenderable, Transform, BoxCollider>().each(
                    [&cam](LineRenderable&, Transform& tr, BoxCollider& box) {
                        tr.position = cam.get_position();
                        const glm::vec3 size{0.1F, 1.0F, 0.1F};
                        box.min = tr.position - size;
                        box.max = tr.position + size;
                    }
            );
        }

        PointLight& pl = m_Registry->get<PointLight>(m_Registry->view<PointLight>().front());
        pl.position = cam.get_position() + glm::vec3{0, 1.5F, -0.8F};

        m_Registry->view<PointLight>().each(
                [&cam](PointLight& light) {
                    static size_t index;
                    index = 0;
                    srand(index);
                    ++index;
                    float radius = -8 + rand() % 16;
                    light.position = cam.get_position() + glm::vec3{radius, radius * 0.5F, radius};
                }
        );

        int width, height;
        SDL_GetWindowSize(m_Window->handle, &width, &height);
        glViewport(0, 0, width, height);
        m_Context->clear();

        // Moving Around
        float speed = 20.0F;
        if (InputSystem::is_key_pressed(SDLK_1)) speed = 100.0F;
        float vel = speed * m_Timing->delta;

        if (InputSystem::is_key_pressed(SDLK_w)) cam.translate_forward(vel);
        if (InputSystem::is_key_pressed(SDLK_s)) cam.translate_forward(-vel);
        if (InputSystem::is_key_pressed(SDLK_a)) cam.translate_horizontal(-vel);
        if (InputSystem::is_key_pressed(SDLK_d)) cam.translate_horizontal(vel);

        glm::vec3 pos = cam.get_position();
        cam.set_position(pos);

        // Capture Mouse
        static bool is_relative_mode = false;
        if (InputSystem::is_key_released(SDLK_ESCAPE)) {
            is_relative_mode = !is_relative_mode;
            SDL_SetRelativeMouseMode(is_relative_mode ? SDL_TRUE : SDL_FALSE);
        }

        // Looking Around
        if (is_relative_mode) {
            glm::vec2 mvel = InputSystem::get_mouse_vel() * m_Timing->delta * 2.0F;
            cam.look(mvel);
        }

        if (InputSystem::is_key_pressed(SDLK_LEFT)) cam.look_horizontal(-m_Timing->fixed);
        if (InputSystem::is_key_pressed(SDLK_RIGHT)) cam.look_horizontal(m_Timing->fixed);
        if (InputSystem::is_key_pressed(SDLK_UP)) cam.look_vertical(m_Timing->fixed);
        if (InputSystem::is_key_pressed(SDLK_DOWN)) cam.look_vertical(-m_Timing->fixed);

        static bool is_movement_xz = false;
        if (InputSystem::is_key_released(SDLK_1)) {
            is_movement_xz = !is_movement_xz;
            cam.set_move_xz(is_movement_xz);
        }

        if (InputSystem::is_key_released(SDLK_r)) cam.look_at(glm::vec3{0});

        if (ImGui::Begin("Debug")) {
            ImGui::Text("W => %s", InputSystem::is_key_pressed(SDLK_w) ? "Pressed" : "Released");
            ImGui::Text("A => %s", InputSystem::is_key_pressed(SDLK_a) ? "Pressed" : "Released");
            ImGui::Text("S => %s", InputSystem::is_key_pressed(SDLK_s) ? "Pressed" : "Released");
            ImGui::Text("D => %s", InputSystem::is_key_pressed(SDLK_d) ? "Pressed" : "Released");

            auto mvel = InputSystem::get_mouse_vel();
            ImGui::Text("MVel => ( %.4f, %.4f )", mvel.x, mvel.y);

            glm::vec3 position = cam.get_position();
            glm::vec3 rotation = cam.get_rotate();
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
//    ctx.emplace_app<OctreeVisualisationApp>();
    ctx.emplace_app<PhysicsSystem>();
    ctx.emplace_app<RenderApplication>();
    if (!ctx.start()) return -1;
    return 0;
}