//
// Date       : 17/02/2024
// Project    : octvis
// Author     : -Ry
//

#ifndef OCTVIS_PHYSICSSYSTEM_H
#define OCTVIS_PHYSICSSYSTEM_H

#include "Application.h"
#include "Octree.h"

namespace octvis {

    //############################################################################//
    // | COMPONENTS |
    //############################################################################//

    struct PhysicsTag {};
    struct ColliderTag {};
    struct ImmovableTag {};

    // @off

    struct CollisionTracker {
        // Colliding Entity, Collision Checks, Actual Collisions
        std::function<void(entt::entity, size_t, size_t)> callback;

        size_t num_collision_tests; // Number of times this entity has been checked for a collision
        size_t num_collisions;      // Actual number of collisions

        inline void operator()(entt::entity entity) const noexcept {
            callback(entity, num_collision_tests, num_collisions);
        }
    };

    struct BoxCollider {
        glm::vec3 min;
        glm::vec3 max;
    };

    struct SphereCollider {
        glm::vec3 centre;
        float     radius;
    };

    struct RigidBody {
        glm::vec3 acceleration = glm::vec3{0.0F};
        glm::vec3 velocity     = glm::vec3{0.0F};
        float     mass         = 5.0F;
        float     friction     = 0.05F;
    };

    // @on

    //############################################################################//
    // | UTILITY FUNCTIONS |
    //############################################################################//

    inline glm::vec3 compute_forward(const glm::vec3& pos, float yaw, float pitch) noexcept {
        // Stolen from the Camera class
        glm::vec3 forward{
                std::cos(yaw) * std::cos(pitch),
                std::sin(pitch),
                std::sin(yaw) * std::cos(pitch)
        };
        return glm::normalize(forward);
    };

    //############################################################################//
    // | PHYSICS SYSTEM |
    //############################################################################//

    class PhysicsSystem : public Application {

      public:
        constexpr static float GRAVITY = 10.0F;
        constexpr static float DAMPING = 0.98F;

      private:
        float m_PhysicsDuration;
        float m_CollisionDuration;
        size_t m_CollisionTests;
        size_t m_CollisionsResolved;
        int m_FixedUpdateFramerate;

      private: // @off
        using Node = Octree<entt::entity>::Node;
        Octree<entt::entity> m_Octree;
        int                  m_OctreeDepth;
        float                m_OctreeSize;
        glm::vec3            m_OctreeCentre;
      // @on

      private:
        bool m_UseOctree = true;
        bool m_RenderAsBoundingBox = false;
        bool m_VisualiseOctree = false;
        bool m_VisualiseCurrentOctant = false;
        bool m_VisualiseMainOctant = false;
        entt::entity m_TreeEntity = entt::null;

      public:
        PhysicsSystem() noexcept;

        virtual ~PhysicsSystem() noexcept;

      public:
        virtual void on_start() noexcept override;

        virtual void on_update() noexcept override;

        virtual void on_fixed_update() noexcept override;

      private:
        void simulate_physics() noexcept;

        void resolve_collisions_linearly() noexcept;

        void resolve_collisions_accelerated() noexcept;

      private:
        bool is_colliding(entt::entity lhs, entt::entity rhs) noexcept;

        void resolve_collision(
                entt::entity e0, RigidBody& rb0, Transform& tr0,
                entt::entity e1, RigidBody& rb1, Transform& tr1
        ) noexcept;
    };

} // octvis

#endif
