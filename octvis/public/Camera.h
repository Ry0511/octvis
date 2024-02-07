//
// Date       : 11/01/2024
// Project    : octvis
// Author     : -Ry
//

#ifndef OCTVIS_CAMERA_H
#define OCTVIS_CAMERA_H

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <algorithm>

namespace octvis {

    //@off
    constexpr float PI = 3.141592653589793F;
    //@on

    class Camera {

        // NOTE: Roll is unused and mostly untested

      private:
        glm::vec3 m_Position; // World Space Position
        glm::vec3 m_Rotate;   // Yaw, Pitch, and Roll

      private: // Computed from Rotate
        glm::vec3 m_Forward;
        glm::vec3 m_Right;
        glm::vec3 m_Up;
        bool m_IsMovementXZ;

      private:
        glm::mat4 m_Projection;
        glm::mat4 m_ViewMatrix;

      public:
        Camera() noexcept;
        ~Camera() noexcept = default;

      public:
        Camera(const Camera&) noexcept = default;
        Camera(Camera&&) noexcept = default;
        Camera& operator =(const Camera&) noexcept = default;
        Camera& operator =(Camera&&) noexcept = default;

      public: // @off
        inline const glm::vec3& get_position()    const noexcept { return m_Position;   }
        inline const glm::vec3& get_rotate()      const noexcept { return m_Rotate;     }
        inline const glm::vec3& get_up()          const noexcept { return m_Up;         }
        inline const glm::vec3& get_forward()     const noexcept { return m_Forward;    }
        inline const glm::vec3& get_right()       const noexcept { return m_Right;      }
        inline const glm::mat4& get_projection()  const noexcept { return m_Projection; }
        inline const glm::mat4& get_view_matrix() const noexcept { return m_ViewMatrix; }
      // @on

      public:
        inline void set_projection(float fov, float near, float far, float aspect) noexcept {
            m_Projection = glm::perspective(fov, aspect, near, far);
        }

      public:
        inline void set_position(const glm::vec3& pos) noexcept {
            m_Position = pos;
            compute_view_matrix();
        };
        inline void set_rotate(const glm::vec3& rotate) noexcept {
            m_Rotate = rotate;
            normalise_rotation();
            compute_view_matrix();
        };
        inline void look_at(const glm::vec3& target) noexcept {

            // Compute Vectors
            m_Forward = glm::normalize(target - m_Position);
            m_Right = glm::normalize(glm::cross(m_Forward, glm::vec3(0, 1, 0)));
            m_Up = glm::normalize(glm::cross(m_Right, m_Forward));

            // Extract Yaw & Pitch
            m_Rotate.x = glm::atan(m_Forward.z, m_Forward.x);
            m_Rotate.y = glm::asin(m_Forward.y);

            // Compute the new view matrix
            m_ViewMatrix = glm::lookAt(m_Position, target, m_Up);
        }
        inline void set_up(const glm::vec3& up) noexcept {
            m_Up = up;
            compute_view_matrix();
        };
        inline void set_move_xz(bool is_movement_xz = true) noexcept {
            m_IsMovementXZ = is_movement_xz;
        }

      public: // Controlling the camera
        void translate_forward(float dt) noexcept;
        void translate_horizontal(float dt) noexcept;
        void translate(const glm::vec2& dt) noexcept;

      public: // Looking Around
        void look_vertical(float dt) noexcept;
        void look_horizontal(float dt) noexcept;
        void look_pitch(float dt) noexcept;
        void look(const glm::vec3& dt) noexcept;
        void look(const glm::vec2& dt) noexcept;

      public: // Really should be private
        inline void compute_view_matrix() noexcept {

            // m_Rotate is in radians
            m_Forward = glm::vec3{
                    std::cos(m_Rotate.x) * std::cos(m_Rotate.y),
                    -std::sin(m_Rotate.y), // Otherwise look axis is inverted
                    std::sin(m_Rotate.x) * std::cos(m_Rotate.y)
            };
            m_Forward = glm::normalize(m_Forward);

            // Recompute Right and Up
            m_Right = glm::normalize(glm::cross(m_Forward, glm::vec3(0, 1, 0)));
            m_Up = glm::normalize(glm::cross(m_Right, m_Forward));

            m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Forward, m_Up);

        }
        inline void normalise_rotation() noexcept {

            // Normalises to a range preserving a wrap-around-nature.
            constexpr auto normalise = [](float v, float min, float max) -> float {
                float range = max - min;
                while (v < min) v += range;
                while (v > max) v -= range;
                return v;
            };

            // Converting Degrees to Radians
            constexpr auto to_radians = [](double degrees) -> double {
                return degrees * (PI / 180.0F);
            };

            // Typical Values; @off
            constexpr float YAW_MIN   = to_radians(-180.0F), YAW_MAX   = to_radians(180.0F);
            constexpr float PITCH_MIN = to_radians(-89.0F),  PITCH_MAX = to_radians(89.0F);
            constexpr float ROLL_MIN  = to_radians(-180.0F), ROLL_MAX  = to_radians(180.0F);

            // Normalise or Clamp
            m_Rotate.x = normalise(m_Rotate.x, YAW_MIN, YAW_MAX);

            #if 0 // Unconstrained Pitch
                m_Rotate.y = normalise(m_Rotate.y, PITCH_MIN, PITCH_MAX);
            #else // Constrained Pitch
                m_Rotate.y = std::clamp(m_Rotate.y, PITCH_MIN, PITCH_MAX);
            #endif

            // Roll is Unused, but it did make me dizzy messing around with it :)
            m_Rotate.z = normalise(m_Rotate.z, ROLL_MIN, ROLL_MAX);
            // @on
        }
    };

} // octvis

#endif
