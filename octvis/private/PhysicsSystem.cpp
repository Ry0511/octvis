//
// Date       : 17/02/2024
// Project    : octvis
// Author     : -Ry
//

#include "PhysicsSystem.h"

#include "Application.h"
#include "RenderApplication.h"
#include "Utility.h"

#include <execution>

namespace octvis {

    PhysicsSystem::PhysicsSystem() noexcept: Application("Physics System"),
                                             m_FixedUpdateFramerate(60),
                                             m_OctreeDepth(2),
                                             m_OctreeSize(1024.0F),
                                             m_Octree(glm::vec3{0.0F}, 1024.0F, 2) {

    }

    PhysicsSystem::~PhysicsSystem() noexcept {

    }

    //############################################################################//
    // | APPLICATION OVERRIDEN |
    //############################################################################//

    void PhysicsSystem::on_start() noexcept {

        constexpr size_t ENTITY_COUNT = 32;
        for (int i = 0; i < ENTITY_COUNT; ++i) {
            entt::entity e = m_Registry->create();

            m_Registry->emplace<PhysicsTag>(e);
            m_Registry->emplace<ColliderTag>(e);
            m_Registry->emplace<RenderableTag>(e);

            auto& rb = m_Registry->emplace<RigidBody>(e);
            auto& sc = m_Registry->emplace<SphereCollider>(e);
            auto& trans = m_Registry->emplace<Transform>(e);

            auto& renderable = m_Registry->emplace<Renderable>(e);
            m_Registry->emplace<ModelMatrix>(e);

            float r = (20 + rand() % 80) / 100.0F;
            float g = (20 + rand() % 80) / 100.0F;
            float b = (20 + rand() % 80) / 100.0F;

            renderable.colour = glm::vec4{r, g, b, 1.0F};
            renderable.model_id = 3;
            renderable.use_depth_test = true;
            renderable.use_wireframe = false;
            renderable.use_face_culling = true;

            trans.position = glm::vec3{-128 + rand() % 256, 32 + rand() % 128, -128 + rand() % 256};

            float s = (rand() % 1200) / 100.0F;
            rb.mass = 3.0F + 1.0F * s;
            rb.friction = (70 + rand() % 30) / 100.0F;
            trans.scale = glm::vec3{s};
            sc.centre = trans.position;
            sc.radius = 1.0F * s;
        }

        for (int i = 0; i < ENTITY_COUNT; ++i) {
            entt::entity e = m_Registry->create();

            m_Registry->emplace<PhysicsTag>(e);
            m_Registry->emplace<ColliderTag>(e);
            m_Registry->emplace<RenderableTag>(e);

            auto& rb = m_Registry->emplace<RigidBody>(e);
            auto& sc = m_Registry->emplace<BoxCollider>(e);
            auto& trans = m_Registry->emplace<Transform>(e);

            auto& renderable = m_Registry->emplace<Renderable>(e);
            m_Registry->emplace<ModelMatrix>(e);

            float r = (20 + rand() % 80) / 100.0F;
            float g = (20 + rand() % 80) / 100.0F;
            float b = (20 + rand() % 80) / 100.0F;

            renderable.colour = glm::vec4{r, g, b, 1.0F};
            renderable.model_id = 2;
            renderable.use_depth_test = true;
            renderable.use_wireframe = false;
            renderable.use_face_culling = true;

            trans.position = glm::vec3{-128 + rand() % 256, 32, -128 + rand() % 256};

            float s0 = (350 + rand() % 850) / 100.0F;
            float s1 = (350 + rand() % 850) / 100.0F;
            float s2 = (350 + rand() % 850) / 100.0F;
            rb.mass = 3.0F + 1.0F * ((s0 + s1 + s2) / 3.0F) * 0.5F;
            rb.friction = 0.9F;
            trans.scale = glm::vec3{s0, s1, s2};

            glm::vec3 half_scale = trans.scale;
            sc.min = trans.position - half_scale;
            sc.max = trans.position + half_scale;
        }

        m_TreeEntity = m_Registry->create();
        m_Registry->emplace<LineRenderable>(m_TreeEntity);
    }

    void PhysicsSystem::on_fixed_update() noexcept {

        m_CollisionTests = 0;
        m_CollisionsResolved = 0;

        {
            auto group = m_Registry->group<CollisionTracker>();
            group.each(
                    [](CollisionTracker& tracker) {
                        tracker.num_collision_tests = 0;
                        tracker.num_collisions = 0;
                    }
            );
        }

        auto rectify_entity_positions = [this]() {

            auto sphere_group = m_Registry->group<SphereCollider>(entt::get<Transform>);
            sphere_group.each(
                    [](SphereCollider& collider, Transform& trans) {
                        trans.position.y = std::max(trans.position.y, collider.radius);
                        collider.centre = trans.position;
                        collider.centre = trans.position;
                    }
            );

            auto box_group = m_Registry->group<BoxCollider>(entt::get<Transform>);
            box_group.each(
                    [](BoxCollider& sc, Transform& trans) {
                        trans.position.y = std::max(trans.position.y, trans.scale.y);
                        sc.min = trans.position - trans.scale;
                        sc.max = trans.position + trans.scale;
                    }
            );

        };

        rectify_entity_positions();

        // Compute Physics
        start_timer();
        simulate_physics();
        m_PhysicsDuration = elapsed();

        // Resolve Collisions
        start_timer();
        if (m_UseOctree) {
            resolve_collisions_accelerated();
        } else {
            resolve_collisions_linearly();
        }
        m_CollisionDuration = elapsed();

        rectify_entity_positions();
    }

    void PhysicsSystem::on_update() noexcept {
        srand(m_Timing->delta_ticks + m_Timing->fixed_ticks);

        if (ImGui::Begin("Physics System")) {

            ImGui::SeparatorText("Physics Timings");
            ImGui::Text("Physics Update      %.4f", m_PhysicsDuration);
            ImGui::Text("Collision Update    %.4f", m_CollisionDuration);
            ImGui::Text("Collision Tests     %llu", m_CollisionTests);
            ImGui::Text("Collisions Resolved %llu", m_CollisionsResolved);

            ImGui::SeparatorText("General");
            if (ImGui::SliderInt(
                    "Fixed Update Framerate",
                    &m_FixedUpdateFramerate,
                    20,
                    300,
                    "%d",
                    ImGuiSliderFlags_AlwaysClamp
            )) {
                m_Timing->fixed = 1.0F / float(m_FixedUpdateFramerate);
            }
            ImGui::Checkbox("Accelerate Collisions with Octree?", &m_UseOctree);
            ImGui::Checkbox("Render all as Wireframe Box?", &m_RenderAsBoundingBox);

            m_Registry->view<CameraTag, LineRenderable>().each(
                    [](LineRenderable& line) {
                        ImGui::Checkbox("Render Collision Lines?", &line.enabled);
                    }
            );

            ImGui::SeparatorText("Octree Controls");
            ImGui::SliderFloat("Octree Size", &m_OctreeSize, 128.0F, 2048.0F, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderInt("Octree Depth", &m_OctreeDepth, 0, 4, "%d", ImGuiSliderFlags_AlwaysClamp);

            ImGui::Checkbox("Visualise Entire Octree?", &m_VisualiseOctree);
            if (!m_VisualiseOctree) {
                ImGui::Checkbox("Visualise Current Octant?", &m_VisualiseCurrentOctant);
                ImGui::Checkbox("Visualise Main Octant?", &m_VisualiseMainOctant);
            }
        }
        ImGui::End();

        // Render Wireframe of Renderables
        auto view = m_Registry->view<Renderable, Transform>();
        for (entt::entity e0 : view) {
            struct Backup { Renderable re; Transform tr; };
            Renderable& re = view.get<Renderable>(e0);
            Transform& tr = view.get<Transform>(e0);
            Backup* back = m_Registry->try_get<Backup>(e0);

            if (m_RenderAsBoundingBox) {
                if (back == nullptr) {
                    Backup& backup = m_Registry->emplace<Backup>(e0);
                    backup.re = re;
                    backup.tr = tr;
                }
                re.model_id = 2;
                re.use_wireframe = true;
                re.use_face_culling = false;
                re.use_depth_test = false;
                re.colour = glm::vec4{1.0F, 0.0F, 1.0F, 1.0F};
                glLineWidth(2.0F);

                // Restore
            } else if (back != nullptr) {
                re = back->re;
                tr.scale = back->tr.scale;
                tr.rotation = back->tr.rotation;
                glLineWidth(1.0F);
            }
        }

        if (InputSystem::is_key_released(SDLK_t)) {
            auto& cam = m_Registry->get<Camera>(m_Registry->view<Camera>().front());
            m_Registry->view<RigidBody, Transform>().each(
                    [](RigidBody& rb, Transform& tr) {
                        tr.position.y = (64 + rand() % 512);
                        rb.acceleration = glm::vec3{
                                float(-100 + rand() % 200) * 0.01F,
                                float(-100 + rand() % 200) * 0.01F,
                                float(-100 + rand() % 200) * 0.01F
                        } * 100.0F;
                    }
            );
        }

        LineRenderable& line = m_Registry->get<LineRenderable>(m_TreeEntity);
        line.vertices.clear();
        line.enabled = false;

        constexpr auto insert_lines_for_cube = [](
                LineRenderable& line,
                const glm::vec3& len,
                const glm::vec3& centre
        ) {
            glm::vec3 pts[]{
                    centre + glm::vec3(-len.x, -len.y, len.z),
                    centre + glm::vec3(len.x, -len.y, len.z),
                    centre + glm::vec3(len.x, -len.y, -len.z),
                    centre + glm::vec3(-len.x, -len.y, -len.z),
                    centre + glm::vec3(-len.x, len.y, len.z),
                    centre + glm::vec3(len.x, len.y, len.z),
                    centre + glm::vec3(len.x, len.y, -len.z),
                    centre + glm::vec3(-len.x, len.y, -len.z)
            };
            line.vertices.insert(
                    line.vertices.end(),
                    {pts[0], pts[1], pts[1], pts[2], pts[2], pts[3], pts[3], pts[0],
                     pts[4], pts[5], pts[5], pts[6], pts[6], pts[7], pts[7], pts[4],
                     pts[0], pts[4], pts[1], pts[5], pts[2], pts[6], pts[3], pts[7]}
            );
        };

        if (m_VisualiseOctree) {

            m_VisualiseCurrentOctant = false;
            m_VisualiseMainOctant = false;
            line.enabled = true;
            line.vertices.clear();

            m_Octree.for_each(
                    [&line, insert_lines_for_cube](Node& node) {
                        insert_lines_for_cube(line, glm::vec3{node.size}, node.centre);
                    }
            );

        } else {

            if (m_VisualiseCurrentOctant) {
                line.enabled = true;

                Camera& cam = m_Registry->get<Camera>(m_Registry->view<Camera>().front());
                const glm::vec3& pos = cam.get_position();
                Node* closest = nullptr;
                float dist = std::numeric_limits<float>{}.max();
                m_Octree.for_each(
                        [&pos, &dist, &closest](Node& node) {
                            float d = glm::distance(pos, node.centre);
                            if (d < dist) {
                                closest = &node;
                                dist = d;
                            }
                        }
                );

                if (closest != nullptr) {
                    insert_lines_for_cube(line, glm::vec3{closest->size}, closest->centre);
                }
            }

            if (m_VisualiseMainOctant) {
                line.enabled = true;
                insert_lines_for_cube(line, glm::vec3{m_Octree.size()}, m_Octree.centre());
            }
        }

    }

    //############################################################################//
    // | PHYICS SIMULATION & COLLISION RESOLUTION |
    //############################################################################//

    void PhysicsSystem::simulate_physics() noexcept {

        start_timer();
        auto physics_group = m_Registry->group<PhysicsTag, RigidBody>(entt::get<Transform>);

        constexpr auto compute_physics_for = [](
                PhysicsSystem& self,
                RigidBody& rb,
                Transform& transform
        ) -> void {

            // Apply Gravity, Friction, and an arbitrary value Damping.
            rb.acceleration.y -= GRAVITY;
            rb.velocity += rb.acceleration;
            rb.velocity -= rb.velocity * rb.friction;

            // Clamp Velocity
            constexpr float MAX_VELOCITY = 1000.0F;
            rb.velocity = glm::clamp(rb.velocity, -MAX_VELOCITY, MAX_VELOCITY);

            // Apply Velocity
            transform.position += rb.velocity * self.m_Timing->fixed;
            rb.acceleration *= DAMPING;
        };

        #define PHYSICS_SYSTEM_PARALLEL_PHYSICS
        #ifdef PHYSICS_SYSTEM_PARALLEL_PHYSICS
        std::for_each(
                std::execution::par_unseq,
                physics_group.begin(),
                physics_group.end(),
                [this, compute_physics_for](entt::entity e) -> void {
                    RigidBody& rb = m_Registry->get<RigidBody>(e);
                    Transform& transform = m_Registry->get<Transform>(e);
                    compute_physics_for(*this, rb, transform);
                }
        );

        #else

        physics_group.each(
                [this, compute_physics_for](RigidBody& rb, Transform& transform) -> void {
                    compute_physics_for(*this, rb, transform);
                }
        );

        #endif

    }

    //############################################################################//
    // | METHODS |
    //############################################################################//

    void increment_tests(CollisionTracker* tracker, entt::entity entity) {
        if (tracker != nullptr) {
            tracker->num_collision_tests++;
            (*tracker)(entity);
        }
    }

    void increment_collisions(CollisionTracker* tracker) {
        if (tracker != nullptr) {
            tracker->num_collisions++;
        }
    }

    void PhysicsSystem::resolve_collisions_linearly() noexcept {

        auto group = m_Registry->view<ColliderTag, RigidBody, Transform>();

        for (auto [e0, rb0, tr0] : group.each()) {

            CollisionTracker* e0_tracker = m_Registry->try_get<CollisionTracker>(e0);

            for (auto [e1, rb1, tr1] : group.each()) {
                if (e0 == e1) continue;

                CollisionTracker* e1_tracker = m_Registry->try_get<CollisionTracker>(e1);

                increment_tests(e0_tracker, e1);
                increment_tests(e1_tracker, e0);

                if (!is_colliding(e0, e1))continue;

                increment_collisions(e0_tracker);
                increment_collisions(e1_tracker);
                resolve_collision(e0, rb0, tr0, e1, rb1, tr1);
            }
        }
    }

    void PhysicsSystem::resolve_collisions_accelerated() noexcept {

        auto group = m_Registry->group<ColliderTag>(entt::get<RigidBody, Transform>);

        // Re-create Tree
        m_OctreeCentre = glm::vec3{0.0F, m_OctreeSize, 0.0F};
        m_Octree.rebuild(m_OctreeCentre, m_OctreeSize, m_OctreeDepth);
        Octree<entt::entity>& tree = m_Octree;

        constexpr auto get_bounds = [](SphereCollider* sphere, BoxCollider* box) {
            BoxCollider collider{};

            if (sphere != nullptr) {
                collider.min = sphere->centre - (sphere->radius * 1.5F);
                collider.max = sphere->centre + (sphere->radius * 1.5F);

            } else if (box != nullptr) {
                collider.min = box->min - 5.0F;
                collider.max = box->max + 5.0F;
            }

            return collider;
        };

        // Populate Tree
        for (auto [e0, rb0, tr0] : group.each()) {
            SphereCollider* sphere = m_Registry->try_get<SphereCollider>(e0);
            BoxCollider* box = m_Registry->try_get<BoxCollider>(e0);
            BoxCollider bounds = get_bounds(sphere, box);
            tree.insert(e0, bounds.min, bounds.max);
        }

        // Collision Detection & Resolution Using Octree
        for (auto [e0, rb0, tr0] : group.each()) {
            SphereCollider* sphere = m_Registry->try_get<SphereCollider>(e0);
            BoxCollider* box = m_Registry->try_get<BoxCollider>(e0);
            BoxCollider bounds = get_bounds(sphere, box);

            Node* node = tree.search(
                    [&bounds](Node& node) {
                        glm::vec3 min, max;
                        min = node.centre - node.size;
                        max = node.centre + node.size;
                        return collision::box_intersects_box(
                                bounds.min, bounds.max,
                                min, max
                        );
                    }
            );

            if (node == nullptr) continue;

            CollisionTracker* e0_tracker = m_Registry->try_get<CollisionTracker>(e0);

            node->for_each(
                    [this, &group, e0, &rb0, &tr0, e0_tracker](entt::entity e1) {
                        if (e0 == e1) return;

                        CollisionTracker* e1_tracker = m_Registry->try_get<CollisionTracker>(e1);

                        increment_tests(e0_tracker, e1);
                        increment_tests(e1_tracker, e0);

                        if (!this->is_colliding(e0, e1)) return;

                        increment_collisions(e0_tracker);
                        increment_collisions(e1_tracker);

                        RigidBody& rb1 = group.get<RigidBody>(e1);
                        Transform& tr1 = group.get<Transform>(e1);
                        resolve_collision(
                                e0, rb0, tr0,
                                e1, rb1, tr1
                        );
                    }
            );
        }

    }

//############################################################################//
// | COLLISION HANDLING |
//############################################################################//

    inline void resolve_sphere_vs_sphere(
            SphereCollider& c0, RigidBody& rb0, Transform& tr0,
            SphereCollider& c1, RigidBody& rb1, Transform& tr1
    ) {
        glm::vec3 collision_normal = glm::normalize(tr1.position - tr0.position);
        float distance = glm::distance(tr0.position, tr1.position);
        float radius_sum = c0.radius + c1.radius;
        float overlap = radius_sum - distance;

        if (overlap <= 0.0) return;

        // Resolve Collision
        glm::vec3 separation_vec = collision_normal * overlap * 0.5F;
        tr0.position -= (separation_vec + 0.05F);
        tr1.position += (separation_vec + 0.05F);

        c0.centre = tr0.position;
        c1.centre = tr1.position;

        // Collision Impulse Response
        glm::vec3 relative_velocity = rb1.velocity - rb0.velocity;
        float impulse_magnitude = glm::dot(-relative_velocity, collision_normal) * (1.0F + 1.2F);

        glm::vec3 impulse = impulse_magnitude * collision_normal;
        rb0.velocity -= impulse / rb0.mass;
        rb1.velocity += impulse / rb1.mass;
    }

    inline void resolve_sphere_vs_box(
            SphereCollider& c0, RigidBody& rb0, Transform& tr0,
            BoxCollider& c1, RigidBody& rb1, Transform& tr1
    ) {
        float x = std::max(c1.min.x, std::min(c0.centre.x, c1.max.x));
        float y = std::max(c1.min.y, std::min(c0.centre.y, c1.max.y));
        float z = std::max(c1.min.z, std::min(c0.centre.z, c1.max.z));

        float distance_squared = (x - c0.centre.x) * (x - c0.centre.x)
                                 + (y - c0.centre.y) * (y - c0.centre.y)
                                 + (z - c0.centre.z) * (z - c0.centre.z);

        // Sanity Check
        if (distance_squared <= 0.0F) return;

        // Resolve Collision
        float displacement_magnitude = c0.radius - glm::sqrt(distance_squared);
        glm::vec3 displacement = glm::normalize(c0.centre - glm::vec3(x, y, z)) * displacement_magnitude;
        tr0.position += displacement + 0.01F;

        // Update Colliders
        c0.centre = tr0.position;
        c1.min = tr1.position - tr1.scale;
        c1.max = tr1.position + tr1.scale;
    }

    inline void resolve_box_vs_box(
            BoxCollider& c0, RigidBody& rb0, Transform& tr0,
            BoxCollider& c1, RigidBody& rb1, Transform& tr1
    ) {
        // Calculate the distance between the centers of the boxes
        glm::vec3 distance = tr1.position - tr0.position;

        // Calculate the sum of the half-widths and half-heights
        glm::vec3 half_size_sum = glm::vec3{
                (c0.max.x - c0.min.x) * 0.5F + (c1.max.x - c1.min.x) * 0.5F,
                (c0.max.y - c0.min.y) * 0.5F + (c1.max.y - c1.min.y) * 0.5F,
                (c0.max.z - c0.min.z) * 0.5F + (c1.max.z - c1.min.z) * 0.5F
        };

        // Compute the overlapping distance for each axis
        glm::vec3 overlap = glm::vec3{
                half_size_sum.x - std::abs(distance.x),
                half_size_sum.y - std::abs(distance.y),
                half_size_sum.z - std::abs(distance.z)
        };

        if (overlap.x > 0 && overlap.y > 0 && overlap.z > 0) {
            if (overlap.x < overlap.y && overlap.x < overlap.z) {
                float shift_distance = overlap.x / 2 * (distance.x > 0 ? 1 : -1);
                tr0.position.x -= shift_distance;
                tr1.position.x += shift_distance;
            } else if (overlap.y < overlap.z) {
                float shift_distance = overlap.y / 2 * (distance.y > 0 ? 1 : -1);
                tr0.position.y -= shift_distance;
                tr1.position.y += shift_distance;
            } else {
                float shift_distance = overlap.z / 2 * (distance.z > 0 ? 1 : -1);
                tr0.position.z -= shift_distance;
                tr1.position.z += shift_distance;
            }
        }

        // Adjust Colliders
        c0.min = tr0.position - tr0.scale;
        c0.max = tr0.position + tr0.scale;
        c1.min = tr1.position - tr1.scale;
        c1.max = tr1.position + tr1.scale;
    }

    bool PhysicsSystem::is_colliding(
            entt::entity lhs,
            entt::entity rhs
    ) noexcept {

        SphereCollider* s0 = m_Registry->try_get<SphereCollider>(lhs);
        BoxCollider* b0 = m_Registry->try_get<BoxCollider>(lhs);

        SphereCollider* s1 = m_Registry->try_get<SphereCollider>(rhs);
        BoxCollider* b1 = m_Registry->try_get<BoxCollider>(rhs);

        // Sphere vs Sphere
        if (s0 != nullptr && s1 != nullptr) {
            m_CollisionTests++;
            return collision::sphere_intersects_sphere(s0->centre, s0->radius, s1->centre, s1->radius);

            // Sphere vs Box
        } else if (s0 != nullptr && b1 != nullptr) {
            m_CollisionTests++;
            return collision::box_intersects_sphere(b1->min, b1->max, s0->centre, s0->radius);

            // Box vs Sphere
        } else if (b0 != nullptr && s1 != nullptr) {
            m_CollisionTests++;
            return collision::box_intersects_sphere(b0->min, b0->max, s1->centre, s1->radius);

            // Box vs Box
        } else if (b0 != nullptr && b1 != nullptr) {
            m_CollisionTests++;
            return collision::box_intersects_box(b0->min, b0->max, b1->min, b1->max);
        }

        return false;
    }

    void PhysicsSystem::resolve_collision(
            entt::entity e0, RigidBody& rb0, Transform& tr0,
            entt::entity e1, RigidBody& rb1, Transform& tr1
    ) noexcept {

        SphereCollider* s0 = m_Registry->try_get<SphereCollider>(e0);
        BoxCollider* b0 = m_Registry->try_get<BoxCollider>(e0);

        SphereCollider* s1 = m_Registry->try_get<SphereCollider>(e1);
        BoxCollider* b1 = m_Registry->try_get<BoxCollider>(e1);

        // Sphere vs Sphere
        if (s0 != nullptr && s1 != nullptr) {
            m_CollisionsResolved++;
            resolve_sphere_vs_sphere(
                    *s0, rb0, tr0,
                    *s1, rb1, tr1
            );

            // Sphere vs Box
        } else if (s0 != nullptr && b1 != nullptr) {
            m_CollisionTests++;
            resolve_sphere_vs_box(
                    *s0, rb0, tr0,
                    *b1, rb1, tr1
            );

            // Box vs Sphere
        } else if (b0 != nullptr && s1 != nullptr) {
            m_CollisionsResolved++;
            resolve_sphere_vs_box(
                    *s1, rb1, tr1,
                    *b0, rb0, tr0
            );

            // Box vs Box
        } else if (b0 != nullptr && b1 != nullptr) {
            m_CollisionsResolved++;
            resolve_box_vs_box(
                    *b0, rb0, tr0,
                    *b1, rb1, tr1
            );
        }

    }


} // octvis