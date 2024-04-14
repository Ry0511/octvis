//
// Date       : 17/01/2024
// Project    : octvis
// Author     : -Ry
//

#include "RenderApplication.h"

#include "WavefrontReader.h"
#include "Utility.h"

#include <algorithm>
#include <execution>
#include <chrono>

namespace octvis {

    //############################################################################//
    // | FUNCTIONS |
    //############################################################################//

    std::vector<Vertex> create_wireframe(const std::vector<Vertex>& vertices) {

        std::vector<Vertex> wireframe{};
        OCTVIS_ASSERT(
                vertices.size() > 0 && ((vertices.size() % 3) == 0),
                "Vertices should be triangulated; {}",
                vertices.size()
        );

        using Edge = std::pair<Vertex, Vertex>;
        constexpr auto edge_comparer = [](const Edge& lhs, const Edge& rhs) -> bool {
            const glm::vec3& a = lhs.first.pos;
            const glm::vec3& b = lhs.second.pos;
            const glm::vec3& c = rhs.first.pos;
            const glm::vec3& d = rhs.second.pos;

            if (a.x == c.x && a.y == c.y && a.z == c.z) {
                return (b.x < d.x)
                       || (b.x == d.x && b.y < d.y)
                       || (b.x == d.x && b.y == d.y && b.z < d.z);
            } else {
                return (a.x < c.x)
                       || (a.x == c.x && a.y < c.y)
                       || (a.x == c.x && a.y == c.y && a.z < c.z);
            }
        };
        std::set<Edge, decltype(edge_comparer)> edges{};

        for (size_t i = 0; i < vertices.size() - 3; i += 3) {
            const Vertex& a = vertices[i];
            const Vertex& b = vertices[i + 1];
            const Vertex& c = vertices[i + 2];

            Edge e1 = std::make_pair(a, b);
            Edge e2 = std::make_pair(b, c);
            Edge e3 = std::make_pair(c, a);

            constexpr auto less_than = [](const Edge& lhs, const Edge& rhs) -> bool {
                constexpr auto less = [](const glm::vec3& lhs, const glm::vec3& rhs) -> bool {
                    return glm::length(lhs) < glm::length(rhs);
                };
                if (less(lhs.first.pos, rhs.first.pos)) return true;
                if (less(rhs.first.pos, lhs.first.pos)) return false;
                if (less(lhs.second.pos, rhs.second.pos)) return true;
                if (less(rhs.second.pos, lhs.second.pos)) return false;
                return false;
            };

            edges.insert(less_than(e1, e2) ? e1 : std::make_pair(e1.second, e1.first));
            edges.insert(less_than(e2, e3) ? e2 : std::make_pair(e2.second, e2.first));
            edges.insert(less_than(e3, e1) ? e3 : std::make_pair(e3.second, e3.first));
        }

        for (const auto& [a, b] : edges) {
            wireframe.push_back(a);
            wireframe.push_back(b);
        }

        return wireframe;
    }

    //############################################################################//
    // | STATIC DATA |
    //############################################################################//

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
            m_VAO(),
            m_LineContext(nullptr) {
        s_App = this;
        OCTVIS_TRACE("Render Application Created; '{:p}'", (void*) s_App);
    }

    RenderApplication::~RenderApplication() noexcept {
        delete m_ModelBuffer;
        delete m_InstanceBuffer;
        delete m_UniformBuffer;
        delete m_CommandBuffer;
        delete m_LineContext;
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
        m_UniformBuffer->init<RenderState>(1, nullptr, BufferUsage::DYNAMIC);

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
        debug_init_sphere();

    }

    void RenderApplication::on_update() noexcept {

        // - NOTE -
        // This renderer assumes that the sorting is correct, that is, internally we sort the
        // renderable entities by Render State and then Model ID.
        //

        auto group = m_Registry->group<RenderableTag, Renderable, ModelMatrix>(entt::get<Transform>);

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

        if (group.size() == 0) return;

        // This is trivially made parallel.
        start_timer();
        std::for_each(
                std::execution::par_unseq, group.begin(), group.end(),
                [this](entt::entity entity) {
                    ModelMatrix& mat = m_Registry->get<ModelMatrix>(entity);
                    Transform& trans = m_Registry->get<Transform>(entity);
                    mat.model = trans.as_matrix();
                    mat.normal = glm::mat3{glm::transpose(glm::inverse(mat.model))};
                }
        );
        float model_calc_duration = elapsed();

        // Initialise Uniform Buffer with Camera & Lighting information
        update_render_state();

        struct alignas(64) RenderInfo {
            Renderable state;
            std::vector<MultiDrawCommand> commands;
            std::vector<InstanceData> data;
            size_t count;

            void init_commands(const std::vector<ModelImpl>& models) noexcept {
                commands.resize(models.size());
                for (int i = 0; i < models.size(); ++i) {
                    const ModelImpl& impl = models[i];

                    MultiDrawCommand& cmd = commands[i];
                    cmd.first = impl.begin;
                    cmd.count = impl.vertex_count;
                    cmd.instance_count = 0;
                    cmd.base_instance = 0;
                }
            }
        };

        std::unordered_map<size_t, RenderInfo> flat_map_vec{};
        flat_map_vec.reserve(16);

        start_timer();
        std::for_each(
                std::execution::unseq,
                group.begin(),
                group.end(),
                [&flat_map_vec, this](entt::entity entity) {

                    const Renderable& renderable = m_Registry->get<Renderable>(entity);
                    const ModelMatrix& mat = m_Registry->get<ModelMatrix>(entity);

                    // Add Into Map
                    size_t state_hash = renderable.get_state_hash();
                    const auto& [pair, was_added] = flat_map_vec.try_emplace(state_hash);
                    RenderInfo& info = pair->second;

                    if (was_added) {
                        info.state = renderable;
                        info.init_commands(m_Models);
                    }
                    MultiDrawCommand& cmd = info.commands[renderable.model_id];
                    cmd.instance_count++;

                    InstanceData& data = info.data.emplace_back();
                    data.colour = renderable.colour;
                    data.model = mat.model;
                    data.normal_matrix = mat.normal;
                }
        );
        float renderable_process_duration = elapsed();

        start_timer();
        for (auto& [_, info] : flat_map_vec) {

            size_t begin = 0;
            for (MultiDrawCommand& cmd : info.commands) {
                cmd.base_instance = begin;
                begin += cmd.instance_count;
            }

            render_instance_data(info.state, info.commands, info.data);
        }
        float instanced_render_duration = elapsed();

        start_timer();
        render_lines();
        float line_render = elapsed();

        if (ImGui::Begin("Application Timings")) {
            ImGui::SeparatorText("Render Application");
            ImGui::Text("Model Calculation Duration %.4f", model_calc_duration);
            ImGui::Text("Renderable Processing Duration %.4f", renderable_process_duration);
            ImGui::Text("Group Render Duration %.4f", renderable_process_duration);
            ImGui::Text("Line Render Duration %.4f", line_render);
        }
        ImGui::End();

    }

    void RenderApplication::update_render_state() noexcept {
        RenderState* state = m_UniformBuffer->create_mapping<RenderState>();

        auto camera_group = m_Registry->group<CameraTag, Camera>();

        // Use Defaults
        if (camera_group.empty()) {
            state->projection = glm::infinitePerspective(90.0F, 16.0F / 9.0F, 0.1F);
            state->view = glm::mat4{1};
            state->cam_pos = glm::vec3{0};

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
            if (ImGui::BeginChild("Lighting Info", {0, 120}, true)) {
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
            ImGui::EndChild();
        }
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
            if (ImGui::BeginChild(str_hash.c_str(), {0, 160}, true)) {
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

    void RenderApplication::render_lines() noexcept {
        auto group = m_Registry->group<LineRenderable>();

        if (m_LineContext == nullptr) {

            m_LineContext = new LineRenderContext();
            LineRenderContext& ctx = *m_LineContext;

            ctx.shader->create(
                    "resources/LineVertexShader.glsl",
                    "resources/LineFragmentShader.glsl"
            );

            OCTVIS_ASSERT(ctx.shader->is_valid(), "Line Renderer Shader Invalid!");

            ctx.state->init<LineRenderState>(1, nullptr, BufferUsage::DYNAMIC);
            ctx.lines->reserve(128);
            ctx.commands->reserve(16);
            ctx.line_states->reserve(128);

            ctx.vao->init();
            ctx.vao->bind();

            ctx.vao->attach_buffer(*ctx.lines)
                    .add_interleaved_attributes<glm::vec3>(0)
                    .attach_buffer(*ctx.line_states)
                    .add_interleaved_attributes<glm::vec4, float>(1)
                    .set_divisor_range(1, 3, 1);
            ctx.vao->unbind();
        }

        LineRenderContext& ctx = *m_LineContext;

        {
            LineRenderState* state = ctx.state->create_mapping<LineRenderState>();
            const Camera& main_camera = m_Registry->get<Camera>(m_Registry->view<CameraTag, Camera>().front());
            state->proj = main_camera.get_projection();
            state->view = main_camera.get_view_matrix();
            ctx.state->release_mapping();
        }

        ctx.lines->clear();
        ctx.line_states->clear();
        ctx.commands->clear();

        unsigned int total_line_renderables = 0;
        group.each(
                [&ctx, &total_line_renderables](LineRenderable& line) {
                    if (!line.enabled || line.vertices.empty()) return;
                    MultiDrawCommand cmd{
                            .count = (unsigned int) line.vertices.size(),
                            .instance_count = 1,
                            .first = (unsigned int) ctx.lines->length(),
                            .base_instance = total_line_renderables++
                    };
                    ctx.lines->insert(line.vertices.data(), line.vertices.size());
                    ctx.commands->insert(&cmd, 1);

                    LineState state{
                            .line_width = line.line_width,
                            .colour = line.colour
                    };
                    ctx.line_states->insert(&state, 1);
                }
        );

        if (ctx.commands->empty()) {
            return;
        }

        ctx.shader->activate();
        ctx.state->bind();
        ctx.shader->set_ubo(*ctx.state, 0, "render_state");
        ctx.commands->bind();
        ctx.vao->bind();

        float original_line_width, original_point_size;
        GL_CALL(glGetFloatv(GL_LINE_WIDTH, &original_line_width));
        GL_CALL(glLineWidth(2.0F));
        GL_CALL(glGetFloatv(GL_POINT_SIZE, &original_point_size));
        GL_CALL(glPointSize(4.0F));
        GL_CALL(glDisable(GL_CULL_FACE));
        GL_CALL(glEnable(GL_DEPTH_TEST));
        GL_CALL(glEnable(GL_BLEND));
        GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        GL_CALL(glEnable(GL_LINE_SMOOTH));

        GL_CALL(glMultiDrawArraysIndirect(GL_LINES, nullptr, ctx.commands->length(), 0));

        GL_CALL(glLineWidth(original_line_width));
        GL_CALL(glPointSize(original_point_size));
        GL_CALL(glEnable(GL_CULL_FACE));
        GL_CALL(glDisable(GL_BLEND));
        GL_CALL(glDisable(GL_LINE_SMOOTH));
        GL_CALL(glDisable(GL_DEPTH_TEST));

        ctx.vao->unbind();
        ctx.commands->unbind();
        ctx.state->unbind();
        ctx.shader->deactivate();

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
        int id = m_Models.size();

        ModelImpl& model = m_Models.emplace_back();
        model.vertices = std::move(vertices);
        model.begin = m_ModelBuffer->length();
        model.vertex_count = model.vertices.size();

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

    void RenderApplication::debug_init_sphere() {
        add_model("G:\\Dev\\BlenderModels\\UVUnitSphere.obj");
    }

} // octvis