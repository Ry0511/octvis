//
// Date       : 17/01/2024
// Project    : octvis
// Author     : -Ry
//

#include "RenderApplication.h"

#include "WavefrontReader.h"

#include <algorithm>
#include <execution>
#include <chrono>

namespace octvis {

    RenderApplication* RenderApplication::s_App = nullptr;

    //############################################################################//
    // | CONSTRUCTOR |
    //############################################################################//

    RenderApplication::RenderApplication() noexcept:
            Application("RenderApplication"),
            m_Textures(),
            m_Models(),
            m_ShaderProgram(),
            m_ModelBuffer(nullptr),
            m_InstanceBuffer(nullptr),
            m_UniformBuffer(nullptr),
            m_CommandBuffer(nullptr),
            m_VAO() {
        s_App = this;
        OCTVIS_TRACE("Render Application Created; '{:p}'", (void*) s_App);
    }

    RenderApplication::~RenderApplication() noexcept {
        delete m_ModelBuffer;
        delete m_InstanceBuffer;
        delete m_UniformBuffer;
        delete m_CommandBuffer;
    }

    //############################################################################//
    // | OVERRIDEN METHODS |
    //############################################################################//

    void RenderApplication::on_start() noexcept {
        m_ModelBuffer = new DynamicBuffer<Vertex>(BufferType::ARRAY);
        m_InstanceBuffer = new DynamicBuffer<InstanceData>(BufferType::ARRAY);
        m_UniformBuffer = new Buffer(BufferType::UNIFORM);
        m_CommandBuffer = new Buffer(BufferType::DRAW_INDIRECT);

        // Initialise Buffers
        m_ModelBuffer->reserve(256);
        m_InstanceBuffer->reserve(256);
        m_UniformBuffer->init<RenderState>(1, nullptr, renderer::BufferUsage::DYNAMIC);

        // Initialise Shader
        m_ShaderProgram.create("resources/VertexShader_UBO.glsl", "resources/FragmentShader.glsl");

        // Create VAO mappings
        m_VAO.init();
        m_VAO.bind();
        m_VAO.attach_buffer(*m_ModelBuffer) // Updates once per vertex
             .add_interleaved_attributes<glm::vec3, glm::vec3, glm::vec2, glm::vec4>(0)
             .attach_buffer(*m_InstanceBuffer) // Once Per Instance
             .add_interleaved_attributes<glm::vec4, glm::mat4, glm::mat3>(4)
             .set_divisor_range(4, 12, 1);
        m_VAO.unbind();

        // @ Testing @
        debug_init_triangle();
        debug_init_rect();
        debug_init_cube();

        Vertex* vertices = m_ModelBuffer->create_mapping<Vertex>();

        int id = 0;
        for (const ModelImpl& model : m_Models) {
            OCTVIS_TRACE("NORMAL DATA FOR MODEL {}", id++);
            size_t len = model.begin + model.vertex_count;
            for (size_t i = model.begin; i < (len - 2); i += 3) {
                Vertex& a = vertices[i];
                Vertex& b = vertices[i + 1];
                Vertex& c = vertices[i + 2];

                glm::vec3 normal = glm::normalize(glm::cross(b.pos - a.pos, c.pos - a.pos));

                OCTVIS_TRACE(
                        "Calculated Normal ( {:<2.2f}, {:<2.2f}, {:<2.2f} );"
                        " Stored Normal ( {:<2.2f}, {:<2.2f}, {:<2.2f} )",
                        normal.x, normal.y, normal.z,
                        a.normal.x, a.normal.y, a.normal.z
                );

                a.normal = normal;
                b.normal = normal;
                c.normal = normal;
            }
        }

        m_ModelBuffer->release_mapping();

    }

    void RenderApplication::on_update() noexcept {

        // - NOTE -
        // This renderer assumes that the sorting is correct, that is, internally we sort the
        // renderable entities by Render State and then Model ID.
        //

        auto group = m_Registry->group<RenderableTag, Renderable>(entt::get<Transform>);

        group.sort<Renderable>(
                [](const Renderable& lhs, const Renderable& rhs) {

                    // If the state is the same order internally by model id
                    if (lhs.get_state_hash() == rhs.get_state_hash()) {
                        return lhs.model_id < rhs.model_id;
                    }

                    // If the states differ order by the state
                    return lhs.get_state_hash() < rhs.get_state_hash();
                }
        );

        // Initialise Uniform Buffer with Camera & Lighting information
        update_render_state();

        Renderable active_state;
        active_state.model_id = -1;
        std::vector<MultiDrawCommand> commands(m_Models.size());

        for (int i = 0; i < m_Models.size(); ++i) {
            const ModelImpl& impl = m_Models[i];
            commands[i].first = impl.begin;
            commands[i].count = impl.vertex_count;
        }

        std::vector<InstanceData> instance_data{};
        instance_data.reserve(256);

        std::for_each(
                std::execution::seq,
                group.begin(),
                group.end(),
                [this, &active_state, &commands, &instance_data](const entt::entity e) {
                    const Transform& transform = m_Registry->get<const Transform>(e);
                    const Renderable& renderable = m_Registry->get<const Renderable>(e);

                    // First Iteration
                    if (active_state.model_id == -1) [[unlikely]] {
                        active_state = renderable;

                        // State Switched; Start Rendering this state
                    } else if (active_state.get_state_hash() != renderable.get_state_hash()) {

                        // Instance Data
                        unsigned int begin = 0;
                        for (MultiDrawCommand& cmd : commands) {

                            // Skip Empty Models
                            if (cmd.instance_count == 0) {
                                continue;
                            }

                            // Instance Stride
                            cmd.base_instance = begin;
                            begin += cmd.instance_count;
                        }

                        render_instance_data(active_state, commands, instance_data);

                        // Initialise for next state (if any)
                        active_state = renderable;
                        for (auto& cmd : commands) cmd.instance_count = 0;
                        instance_data.clear();
                    }

                    // Update State
                    commands[renderable.model_id].instance_count++;

                    // Initialise Instance Data
                    InstanceData& i = instance_data.emplace_back();
                    i.colour = renderable.colour;
                    i.model = transform.as_matrix();
                    i.normal_matrix = glm::mat3{ glm::transpose(glm::inverse(i.model)) };
                }
        );

        // NOTE; this is required otherwise the 'final' state will never be rendered
        if (active_state.model_id != -1) {
            render_instance_data(active_state, commands, instance_data);
        }


    }

    void RenderApplication::update_render_state() noexcept {
        RenderState* state = m_UniformBuffer->create_mapping<RenderState>();

        auto camera_group = m_Registry->group<CameraTag, Camera>();

        // Use Defaults
        if (camera_group.empty()) {
            state->projection = glm::infinitePerspective(90.0F, 16.0F / 9.0F, 0.1F);
            state->view = glm::mat4{ 1 };
            state->cam_pos = glm::vec3{ 0 };

            // Use First Camera
        } else {
            const Camera& main_camera = m_Registry->get<Camera>(camera_group.front());
            state->projection = main_camera.get_projection();
            state->view = main_camera.get_view_matrix();
            state->cam_pos = main_camera.get_position();
        }

        // Set Lights
        state->active_lights = 0;
        m_Registry->view<LightTag, PointLight>().each(
                [state](const PointLight& light) {
                    if (state->active_lights >= RenderState::LIGHT_COUNT) return;
                    state->lights[state->active_lights++] = light;
                }
        );


        if (ImGui::Begin("Renderer Debug")) {
            if (ImGui::BeginChild("Lighting Info", { 0, 120 }, true)) {
                for (int i = 0; i < state->active_lights; ++i) {
                    const PointLight& light = state->lights[i];
                    ImGui::Text(
                            "Light %u\n\t%s",
                            i,
                            std::format(
                                    "{:>3.2f}, {:>3.2f}, {:>3.2f}\n\t"
                                    "{:>3.2f}, {:>3.2f}, {:>3.2f}\n\t"
                                    "{:>3.2f}, {:>3.2f}, {:>3.2f}\n\t"
                                    "{:>3.2f}, {:>3.2f}, {:>3.2f}\n\t"
                                    "{:>3.2f}, {:>3.2f}, {:>3.2f} ({:.2f})\n\t"
                                    "{:>3.2f}, {:>3.2f}, {:>3.2f}\n\t",
                                    light.position.x, light.position.y, light.position.z,
                                    light.colour.x, light.colour.y, light.colour.z,
                                    light.ambient.x, light.ambient.y, light.ambient.z,
                                    light.diffuse.x, light.diffuse.y, light.diffuse.z,
                                    light.specular.x, light.specular.y, light.specular.z, light.shininess,
                                    light.attenuation.x, light.attenuation.y, light.attenuation.z
                            ).c_str()
                    );
                }
            }
        }
        ImGui::EndChild();
        ImGui::End();

        m_UniformBuffer->release_mapping();
    }

    void RenderApplication::render_instance_data(
            const octvis::Renderable& state,
            const std::vector<MultiDrawCommand>& commands,
            const std::vector<InstanceData>& instance_data
    ) noexcept {
        OCTVIS_ASSERT(instance_data.size() > 0, "Rendering 0 instances?");

        if (ImGui::Begin("Renderer Debug")) {
            std::string str_hash = std::to_string(state.get_state_hash());
            if (ImGui::BeginChild(str_hash.c_str(), { 0, 160 }, true)) {
                ImGui::Text("State Hash %llu", state.get_state_hash());
                ImGui::Text("Draw Hash %llu", state.get_hash());
                ImGui::Text(
                        "Instance Data %llu as bytes %llu",
                        instance_data.size(),
                        instance_data.size() * sizeof(InstanceData)
                );

                for (int i = 0; i < commands.size(); ++i) {
                    const MultiDrawCommand& cmd = commands[i];
                    if (cmd.instance_count == 0) continue;
                    ImGui::Text(
                            "%s",
                            std::format(
                                    "First {:>4}, Count {:>4}, Instance Count {:>4}, Base Instance {:>4}",
                                    cmd.first,
                                    cmd.count,
                                    cmd.instance_count,
                                    cmd.base_instance
                            ).c_str()
                    );
                }
            }
            ImGui::EndChild();
        }
        ImGui::End();

        // Upload Draw Commands
        m_CommandBuffer->init<MultiDrawCommand>(commands.size(), commands.data(), BufferUsage::STATIC);

        // Upload Instance Data
        m_InstanceBuffer->reserve(instance_data.size());
        m_InstanceBuffer->clear();
        m_InstanceBuffer->insert(0, instance_data.data(), instance_data.size());

        // Setup State
        if (state.use_depth_test) GL_CALL(glEnable(GL_DEPTH_TEST));
        if (state.use_face_culling) GL_CALL(glEnable(GL_CULL_FACE));
        if (state.use_wireframe) GL_CALL(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));

        // Issue Draw Command
        m_CommandBuffer->bind();

        m_VAO.bind();
        m_ShaderProgram.activate();
        m_ShaderProgram.set_ubo(*m_UniformBuffer, 0, "render_state");

        GL_CALL(glMultiDrawArraysIndirect(GL_TRIANGLES, nullptr, commands.size(), 0));

        m_ShaderProgram.deactivate();
        m_VAO.unbind();
        m_CommandBuffer->unbind();

        // Reset State
        GL_CALL(glDisable(GL_DEPTH_TEST));
        GL_CALL(glDisable(GL_CULL_FACE));
        GL_CALL(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));

    }


    //############################################################################//
    // | UTILITY FUNCTIONS |
    //############################################################################//

    int RenderApplication::add_model(const char* path) {
        // Read File
        WavefrontReader reader{};
        OCTVIS_ASSERT(reader.load_from_path(path), "Failed to load from file '{}'", path);
        return add_model(std::move(reader.get_vertices()));
    }

    int RenderApplication::add_model(std::vector<Vertex>&& vertices) {
        // Get ID
        int id = m_Models.size();
        ModelImpl& model = m_Models.emplace_back();

        // Load Vertex Data into Model
        model.vertices = std::move(vertices);
        model.begin = m_ModelBuffer->length();
        model.vertex_count = model.vertices.size();

        // Upload Vertex Data to GPU
        m_ModelBuffer->insert(model.vertices.data(), model.vertices.size());

        return id;
    }

    //############################################################################//
    // | EVENT FUNCTIONS |
    //############################################################################//

    void RenderApplication::on_renderable_created(entt::basic_registry<>& reg, entt::entity e) noexcept {
        const Renderable& renderable = reg.get<Renderable>(e);
        OCTVIS_TRACE("Renderable Component added to '{}', '{}'", entt::to_integral(e), renderable.model_id);
    }

    void RenderApplication::on_renderable_updated(entt::basic_registry<>& reg, entt::entity e) noexcept {
        const Renderable& renderable = reg.get<Renderable>(e);
        OCTVIS_TRACE("Renderable Component updated '{}', '{}'", entt::to_integral(e), renderable.model_id);
    }

    void RenderApplication::on_renderable_destroyed(entt::basic_registry<>& reg, entt::entity e) noexcept {
        const Renderable& renderable = reg.get<Renderable>(e);
        OCTVIS_TRACE("Renderable Component destroyed to '{}', '{}'", entt::to_integral(e), renderable.model_id);
    }

    //############################################################################//
    // | DEBUG |
    //############################################################################//

    void RenderApplication::debug_init_triangle() {

        // @off
        Vertex vertices[3]{};
        vertices[0] = Vertex{
                .pos     = glm::vec3{ -1.0F, -1.0F, 0.0F },
                .normal  = glm::vec3{ 0.0F, 0.0F, 1.0F },
                .tex_pos = glm::vec2{ 0.0F, 0.0F },
                .colour  = glm::vec4{ 1 }
        };
        vertices[1] = Vertex{
                .pos     = glm::vec3{ 1.0F, -1.0F, 0.0F },
                .normal  = glm::vec3{ 0.0F, 0.0F, 1.0F },
                .tex_pos = glm::vec2{ 1.0F, 0.0F },
                .colour  = glm::vec4{ 1 }
        };
        vertices[2] = Vertex{
                .pos     = glm::vec3{ 0.0F, 1.0F, 0.0F },
                .normal  = glm::vec3{ 0.0F, 0.0F, 1.0F },
                .tex_pos = glm::vec2{ 0.5F, 1.0F },
                .colour  = glm::vec4{ 1 }
        };
        //@on

        add_model(std::vector<Vertex>(vertices, vertices + 3));

    }

    void RenderApplication::debug_init_rect() {
        // @off
        Vertex vertices[6]{};
        vertices[0] = Vertex{
                .pos     = glm::vec3{ -1.0F, -1.0F, 0.0F },
                .normal  = glm::vec3{ 0.0F, 0.0F, 1.0F },
                .tex_pos = glm::vec2{ 0.0F, 0.0F },
                .colour  = glm::vec4{ 1 }
        };
        vertices[1] = Vertex{
                .pos     = glm::vec3{ 1.0F, -1.0F, 0.0F },
                .normal  = glm::vec3{ 0.0F, 0.0F, 1.0F },
                .tex_pos = glm::vec2{ 1.0F, 0.0F },
                .colour  = glm::vec4{ 1 }
        };
        vertices[2] = Vertex{
                .pos     = glm::vec3{ 1.0F, 1.0F, 0.0F },
                .normal  = glm::vec3{ 0.0F, 0.0F, 1.0F },
                .tex_pos = glm::vec2{ 1.0F, 1.0F },
                .colour  = glm::vec4{ 1 }
        };
        vertices[3] = Vertex{
                .pos     = glm::vec3{ -1.0F, -1.0F, 0.0F },
                .normal  = glm::vec3{ 0.0F, 0.0F, 1.0F },
                .tex_pos = glm::vec2{ 0.0F, 0.0F },
                .colour  = glm::vec4{ 1 }
        };
        vertices[4] = Vertex{
                .pos     = glm::vec3{ 1.0F, 1.0F, 0.0F },
                .normal  = glm::vec3{ 0.0F, 0.0F, 1.0F },
                .tex_pos = glm::vec2{ 1.0F, 1.0F },
                .colour  = glm::vec4{ 1 }
        };
        vertices[5] = Vertex{
                .pos     = glm::vec3{ -1.0F, 1.0F, 0.0F },
                .normal  = glm::vec3{ 0.0F, 0.0F, 1.0F },
                .tex_pos = glm::vec2{ 0.0F, 1.0F },
                .colour  = glm::vec4{ 1 }
        };
        //@on

        add_model(std::vector<Vertex>(vertices, vertices + 6));
    }

    void RenderApplication::debug_init_cube() {
        add_model("G:\\Dev\\CLion\\MazeVisualisation\\src\\MazeVisualisation\\Res\\Models\\TexturedCube.obj");
    }

} // octvis