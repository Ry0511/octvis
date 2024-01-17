//
// Date       : 15/01/2024
// Project    : octvis
// Author     : -Ry
//

#ifndef OCTVIS_INPUTSYSTEM_H
#define OCTVIS_INPUTSYSTEM_H

#include "glm/glm.hpp"

#include <variant>
#include <unordered_map>

union SDL_Event;

namespace octvis {

    //############################################################################//
    // | MOUSE EVENTS |
    //############################################################################//

    struct MouseMoved {
        int x, y;
        float xrel, yrel;
    };

    struct MouseScroll {
        int x, y;
        float px, py; // Precise X, and Y
    };

    struct MousePressed {
        int button, click_count, x, y;
    };

    struct MouseReleased {
        int button, click_count, x, y;
    };

    struct KeyPressed {
        int key, mods;
    };

    struct KeyReleased {
        int key, mods;
    };


    //############################################################################//
    // | GENERAL EVENT WRAPPER |
    //############################################################################//

    using EventData = std::variant<
            MouseMoved, MouseScroll, MousePressed, MouseReleased,
            KeyPressed, KeyReleased
    >;

    //############################################################################//
    // | EVENT |
    //############################################################################//

    class InputSystem {

      private: // @off
        using KeyState              = std::variant<KeyPressed, KeyReleased>;
        using KeyStateMap           = std::unordered_map<int, KeyState>;
        using MouseButtonState      = std::variant<MousePressed, MouseReleased>;
        using MouseButtonStateMap   = std::unordered_map<int, MouseButtonState>;

      private:
        friend class Context;
        static InputSystem* s_System;

      private:
        KeyStateMap         m_KeyState;
        MouseButtonStateMap m_MouseButtonState;
        MouseMoved          m_MousePosition;
        MouseScroll         m_MouseScroll;
      // @on

      public:
        InputSystem() noexcept;
        ~InputSystem() noexcept = default;

      public:
        InputSystem(const InputSystem&) noexcept = delete;
        InputSystem(InputSystem&&) noexcept = delete;
        InputSystem& operator =(const InputSystem&) noexcept = delete;
        InputSystem& operator =(InputSystem&&) noexcept = delete;

      private:
        void process_event(const SDL_Event& event) noexcept;
        void reset() noexcept;

      public:
        static bool is_key_pressed(int key) noexcept;
        static bool is_key_released(int key) noexcept;
        static bool is_key_pressed(int key, std::initializer_list<int> mods) noexcept;

      public:
        static bool is_mouse_pressed(int btn) noexcept;
        static bool is_mouse_released(int btn) noexcept;
        static glm::vec2 get_mouse_pos() noexcept;
        static glm::vec2 get_mouse_vel() noexcept;
        static glm::vec2 get_scroll_pos() noexcept;
        static glm::vec2 get_scroll_vel() noexcept;
    };

} // octvis

#endif
