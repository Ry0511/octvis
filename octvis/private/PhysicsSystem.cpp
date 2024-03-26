//
// Date       : 17/02/2024
// Project    : octvis
// Author     : -Ry
//

#include "PhysicsSystem.h"

#include "Application.h"
#include "RenderApplication.h"
#include "Octree.h"
#include "Utility.h"

#include <execution>

namespace octvis {

    PhysicsSystem::PhysicsSystem() noexcept: Application("Physics System") {

    }

    PhysicsSystem::~PhysicsSystem() noexcept {

    }

    //############################################################################//
    // | APPLICATION OVERRIDEN |
    //############################################################################//

    struct BoundBoxTag { entt::entity bound; bool is_tl; };

    void PhysicsSystem::on_start() noexcept {

        constexpr size_t ENTITY_COUNT = 256;
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

            trans.position = glm::vec3{-128 + rand() % 256, 32, -128 + rand() % 256};

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
    }

    void PhysicsSystem::on_fixed_update() noexcept {

        {
            auto group = m_Registry->group<SphereCollider>(entt::get<Transform>);
            group.each(
                    [](SphereCollider& collider, Transform& trans) {
                        trans.position.y = std::max(trans.position.y, collider.radius);
                        collider.centre = trans.position;
                        collider.centre = trans.position;
                    });
        }

        {
            auto group = m_Registry->group<BoxCollider>(entt::get<Transform>);
            group.each(
                    [](BoxCollider& sc, Transform& trans) {
                        trans.position.y = std::max(trans.position.y * 0.5F, trans.scale.y);
                        glm::vec3 half_scale = trans.scale;
                        sc.min = trans.position - half_scale;
                        sc.max = trans.position + half_scale;
                    }
            );
        }

        // Compute Physics
        start_timer();
        simulate_physics();
        m_PhysicsDuration = elapsed();

        // Resolve Collisions
        start_timer();
        resolve_collisions_linearly();
        m_CollisionDuration = elapsed();

    }

    void PhysicsSystem::on_update() noexcept {
        if (ImGui::Begin("Physics System")) {
            ImGui::Text("Physics Update   %.4f", m_PhysicsDuration);
            ImGui::Text("Collision Update %.4f", m_CollisionDuration);
        }
        ImGui::End();

        if (InputSystem::is_key_released(SDLK_t)) {
            auto& cam = m_Registry->get<Camera>(m_Registry->view<Camera>().front());
            m_Registry->view<RigidBody, Transform>().each(
                    [&cam](RigidBody& rb, Transform& tr) {
                        glm::vec3 dir = glm::normalize(cam.get_position() - tr.position);
                        rb.acceleration += dir * rb.mass * 500.0F;
                    }
            );
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

    void PhysicsSystem::resolve_collisions_linearly() noexcept {

        auto group = m_Registry->view<ColliderTag, RigidBody, Transform>();

        m_CollisionTests = 0;
        m_CollisionsResolved = 0;

        for (auto [e0, rb0, tr0] : group.each()) {
            for (auto [e1, rb1, tr1] : group.each()) {
                if (e0 == e1) continue;
                if (!is_colliding(e0, e1)) continue;
                resolve_collision(e0, rb0, tr0, e1, rb1, tr1);
            }
            rb0.acceleration = glm::clamp(rb0.acceleration, -100.0F, 100.0F);
        }
    }

    void PhysicsSystem::resolve_collisions_accelerated() noexcept {

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
        glm::vec3 separation_vec = collision_normal * overlap * 0.5f;
        tr0.position -= separation_vec;
        tr1.position += separation_vec;

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
        tr0.position += displacement;

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
                half_size_sum.x - abs(distance.x),
                half_size_sum.y - abs(distance.y),
                half_size_sum.z - abs(distance.z)
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