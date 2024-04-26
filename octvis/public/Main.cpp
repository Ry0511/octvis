
#include "Logging.h"
#include "Context.h"
#include "Application.h"
#include "RenderApplication.h"
#include "OctreeVisualisationApp.h"
#include "PhysicsSystem.h"
#include "glm/gtc/type_ptr.hpp"

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

        camera.set_projection(90.0F, 0.01F, 2048.0F, 800.0F / 600.0F);
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

            light.specular = glm::vec3{1.0F};
            light.shininess = 256.0F;

            switch ((i+1) % 4) {
                case 0:
                    light.colour = glm::vec3{1.0F};
                    light.attenuation = glm::vec3{0.8F, 0.1F, 0.0F};
                    break;
                case 1:
                    light.colour = glm::vec3{1.0F, 0.0F, 0.0F};
                    light.attenuation = glm::vec3{1.2F, 0.3F, 0.0F};
                    break;
                case 2:
                    light.colour = glm::vec3{0.0F, 1.0F, 0.0F};
                    light.attenuation = glm::vec3{1.8F, 0.3F, 0.0F};
                    break;
                case 3:
                    light.colour = glm::vec3{0.0F, 0.0F, 1.0F};
                    light.attenuation = glm::vec3{0.8F, 0.2F, 0.0F};
                    break;
                default:
                    light.colour = glm::vec3{
                            (30 + (rand() % 90)) * 0.01,
                            (30 + (rand() % 90)) * 0.01,
                            (30 + (rand() % 90)) * 0.01
                    };
                    break;
            }

            light.diffuse *= 1.35F;
        }

        entt::entity player_collider = m_Registry->create();

        m_Registry->emplace_or_replace<CameraTag>(player_collider);
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
        cam.set_projection(90.0F, 0.01F, 2048.0F, float(m_Window->width) / float(m_Window->height));

        {
            m_Registry->view<CameraTag, LineRenderable, Transform, BoxCollider>().each(
                    [&cam](LineRenderable& line, Transform& tr, BoxCollider& box) {
                        tr.position = cam.get_position();
                        const glm::vec3 size{0.25F, 1.0F, 0.25F};
                        box.min = (tr.position - glm::vec3{0, 0.5F, 0}) - size;
                        box.max = (tr.position - glm::vec3{0, 0.5F, 0}) + size;
                    }
            );
        }

        {
            int idx = 0;
            auto pl_view = m_Registry->view<PointLight>();
            size_t count = pl_view.size();

            pl_view.each( // All lights orbit around the camera
                    [this, &cam, count, &idx](PointLight& pl) {
                        constexpr float RADIUS = 32.0F;
                        float theta = 2.0F * 3.1415F * (float) idx / count + m_Timing->theta * 0.25F;
                        glm::vec3 offset = RADIUS * glm::vec3(std::cos(theta), 0.0F, std::sin(theta));
                        pl.position = cam.get_position() + offset;
                        ++idx;
                    }
            );
        }

        int width, height;
        SDL_GetWindowSize(m_Window->handle, &width, &height);
        glViewport(0, 0, width, height);
        m_Context->clear();

        // Moving Around
        float speed = 20.0F;
        if (InputSystem::is_key_pressed(SDLK_2)) speed = 100.0F;
        if (InputSystem::is_key_pressed(SDLK_3)) speed = 200.0F;
        if (InputSystem::is_key_pressed(SDLK_4)) speed = 400.0F;
        float vel = speed * m_Timing->delta;

        glm::vec3 pos = cam.get_position();

        if (InputSystem::is_key_pressed(SDLK_w)) cam.translate_forward(vel);
        if (InputSystem::is_key_pressed(SDLK_s)) cam.translate_forward(-vel);
        if (InputSystem::is_key_pressed(SDLK_a)) cam.translate_horizontal(-vel);
        if (InputSystem::is_key_pressed(SDLK_d)) cam.translate_horizontal(vel);

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

        if (ImGui::Begin("Application")) {

            struct Action {
                const char* key;
                const char* action;

                constexpr Action(const char* k, const char* a) : key(k), action(a) {}
            };

            constexpr Action ACTIONS[]{
                    {"W",   "Move Forward"},
                    {"A",   "Move Left"},
                    {"S",   "Move Backward"},
                    {"D",   "Move Right"},

                    {"",    ""},
                    {"R",   "Force Camera to look at Z 0.0"},
                    {"T",   "Randomise Object Positions (Spheres & Boxes)"},
                    {"ESC", "Start/Stop Capturing the mouse; Locks to main window and hides."},
                    {"1",   "Locks Movement into XZ-Plane"},
                    {"2",   "Holding changes movement speed to 5x"},
                    {"3",   "Holding changes movement speed to 10x"},
                    {"4",   "Holding changes movement speed to 20x"},
            };


            if (ImGui::CollapsingHeader("Controls") && ImGui::BeginTable("Inputs", 2, ImGuiTableFlags_SizingFixedFit)) {
                constexpr float COL0_WIDTH = 50.0F;
                ImGui::TableSetupColumn("col1", ImGuiTableColumnFlags_None, COL0_WIDTH);
                ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                ImGui::TableNextColumn();
                ImGui::Text("Key");
                ImGui::TableNextColumn();
                ImGui::Text("Action");

                for (int i = 0; i < sizeof(ACTIONS) / sizeof(Action); ++i) {
                    const Action& action = ACTIONS[i];
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", action.key);
                    ImGui::TableNextColumn();
                    ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - COL0_WIDTH);
                    ImGui::Text("%s", action.action);
                    ImGui::PopTextWrapPos();
                }
                ImGui::EndTable();
            }

            ImGui::SeparatorText("Update Timings");

            static int average_ticks = 0;
            static double average_fps = 0.0;
            static double average_theta = 0.0;

            if (average_theta > 30.0) {
                average_ticks = 0;
                average_fps = 0.0;
                average_theta = 0.0;
            }

            ++average_ticks;
            average_theta += m_Timing->delta;
            average_fps = double(average_ticks) / average_theta;

            ImGui::Text("Framerate (Single)  %.0f", 1.0F / m_Timing->delta);
            ImGui::Text("Framerate (Average) %.0f", average_fps);
            ImGui::Text("Theta               %.2f", m_Timing->theta);
            ImGui::Text("Delta               %.4f", m_Timing->delta);
            ImGui::Text("Fixed               %.4f", m_Timing->fixed);
            ImGui::Text("Delta Ticks         %llu", m_Timing->delta_ticks);
            ImGui::Text("Fixed Ticks         %llu", m_Timing->fixed_ticks);
            ImGui::Text("Fixed Update Theta  %.4f", m_Timing->fixed_update_total_time);
            ImGui::Text("Delta Update Theta  %.4f", m_Timing->update_total_time);
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