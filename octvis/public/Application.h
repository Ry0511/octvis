//
// Date       : 11/01/2024
// Project    : octvis
// Author     : -Ry
//

#ifndef OCTVIS_APPLICATION_H
#define OCTVIS_APPLICATION_H

#include "Context.h"
#include "Camera.h"
#include "Renderer.h"

#include <memory>

namespace octvis {

    // @off

    struct Transform {
        glm::vec3 position{0.0F}, rotation{0.0F}, scale{1.0F};

        glm::mat4 as_matrix() const noexcept {
            return glm::translate(glm::mat4{1}, position)
                * glm::rotate(glm::mat4{1}, rotation.x, glm::vec3{1, 0, 0})
                * glm::rotate(glm::mat4{1}, rotation.y, glm::vec3{0, 1, 0})
                * glm::rotate(glm::mat4{1}, rotation.z, glm::vec3{0, 0, 1})
                * glm::scale(glm::mat4{1}, scale);
        }
    };

    struct LightTag {};

    struct alignas(16) PointLight /* GLSL ALIGNMENT RULES STD140 */ {
        alignas(16) glm::vec3 position    = glm::vec3{ 0.0F };
        alignas(16) glm::vec3 colour      = glm::vec3{ 1.0F };
        alignas(16) glm::vec3 ambient     = glm::vec3{ 0.05F };
        alignas(16) glm::vec3 diffuse     = glm::vec3{ 0.65F };
        alignas(16) glm::vec3 specular    = glm::vec3{ 0.85F };
        alignas(4)  float     shininess   = 64.0F;
        alignas(16) glm::vec3 attenuation = glm::vec3{ 2.0F, 0.09F, 0.032F };
    };

    inline glm::vec2 get_rotation_to(const glm::vec3& pos, const glm::vec3& at) noexcept {
        glm::vec3 direction = glm::normalize(pos - at);
        return glm::vec2{
            glm::degrees(glm::asin(direction.y)),
            glm::degrees(glm::atan(direction.x, direction.z))
        };
    }

    // @on

    //############################################################################//
    // | APPLICATION |
    //############################################################################//

    class Application {

      public:
        friend Context;

      protected:
        const char* m_AppName;

      protected: // @off
        Context*        m_Context;
        Window*         m_Window;
        Timing*         m_Timing;
        entt::registry* m_Registry;
      // @on

      public:
        Application(const char* app_name) noexcept: m_AppName(app_name) {};
        virtual ~Application() noexcept = default;

      public: // @off
        Application(const Application&)             = default;
        Application(Application&&)                  = default;
        Application& operator =(Application&&)      = default;
        Application& operator =(const Application&) = default;

      public:
        inline virtual void on_start()        noexcept {};
        inline virtual void on_update()       noexcept {};
        inline virtual void on_fixed_update() noexcept {};
        inline virtual void on_finish()       noexcept {};
      // @on

    };

} // octvis

#endif
