//
// Date       : 14/02/2024
// Project    : octvis
// Author     : -Ry
//

#include "OctreeVisualisationApp.h"
#include "RenderApplication.h"

namespace octvis {

    struct OctreeOctantTag {
        const Octree<entt::entity>::Node* node;
    };

    OctreeVisualisationApp::OctreeVisualisationApp()
            : Application("Octree Visualisation"),
              m_Octree(glm::vec3{ 0, 0, 32 }, 64.0F, 1) {

    }

    void OctreeVisualisationApp::on_start() noexcept {
        m_Octree.for_each(
                [this](const Octree<entt::entity>::Node& node) -> void {
                    {
                        entt::entity octant = m_Registry->create();

                        m_Registry->emplace<RenderableTag>(octant);
                        m_Registry->emplace<ModelMatrix>(octant);
                        OctreeOctantTag& octant_tag = m_Registry->emplace<OctreeOctantTag>(octant);
                        Renderable& renderable = m_Registry->emplace<Renderable>(octant);
                        Transform& transform = m_Registry->emplace<Transform>(octant);

                        octant_tag.node = &node;

                        // Initialise Cube Model
                        renderable.model_id = 2;
                        renderable.colour = glm::vec4{
                                (20 + rand() % 80) / 100.0F,
                                (20 + rand() % 80) / 100.0F,
                                (20 + rand() % 80) / 100.0F,
                                1.0F
                        };
                        renderable.use_wireframe = false;
                        renderable.use_depth_test = false;
                        renderable.use_face_culling = true;

                        // Transform to position
                        transform.position = node.centre;
                        transform.scale = glm::vec3{ node.size };
                    }

                    // CENTRE POINT VISUAL
                    {
//                        entt::entity octant_centre = m_Registry->create();
//
//                        m_Registry->emplace<RenderableTag>(octant_centre);
//                        m_Registry->emplace<ModelMatrix>(octant_centre);
//                        Renderable& renderable = m_Registry->emplace<Renderable>(octant_centre);
//                        Transform& transform = m_Registry->emplace<Transform>(octant_centre);
//
//                        // Initialise Cube Model
//                        renderable.model_id = 2;
//                        renderable.colour = glm::vec4{ 1.0F };
//                        renderable.use_wireframe = false;
//                        renderable.use_depth_test = true;
//                        renderable.use_face_culling = true;
//
//                        // Transform to position
//                        transform.position = node.centre;
//                        transform.scale = glm::vec3{ 1.0F };
                    }
                }
        );
    }

    void OctreeVisualisationApp::on_update() noexcept {

        auto group = m_Registry->group<OctreeOctantTag>(entt::get_t<Renderable>());
        const Camera& cam = m_Registry->get<Camera>(m_Registry->view<CameraTag, Camera>().front());

        if (ImGui::Begin("Octree Debug")) {
            ImGui::Text("Octants %llu", group.size());
        }
        ImGui::End();

        group.each(
                [&cam](
                        entt::entity entity,
                        OctreeOctantTag& tag,
                        Renderable& renderable
                ) {
                    if (!tag.node->is_inside(cam.get_position(), 4.0F)) {
                        renderable.use_face_culling = true;
                        renderable.use_wireframe = false;
                        renderable.use_depth_test = true;
                    } else {
                        renderable.use_face_culling = false;
                        renderable.use_wireframe = true;
                        renderable.use_depth_test = false;
                    }
                }
        );

    }

    void OctreeVisualisationApp::on_fixed_update() noexcept {

    }

    void OctreeVisualisationApp::on_finish() noexcept {

    }

} // octvis