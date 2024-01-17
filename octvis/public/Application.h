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

    //############################################################################//
    // | APPLICATION |
    //############################################################################//

    class Application {

      public:
        friend Context;

      protected:
        const char* m_AppName;
        Camera m_Camera;

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
