//
// Date       : 15/01/2024
// Project    : octvis
// Author     : -Ry
//

#include "InputSystem.h"

#include "SDL.h"

#include "Logging.h"

namespace octvis {

    //############################################################################//
    // | CONSTRUCTOR |
    //############################################################################//

    InputSystem* InputSystem::s_System = nullptr;

    InputSystem::InputSystem() noexcept: m_KeyState(), m_MouseButtonState(),
                                         m_MousePosition(), m_MouseScroll() {
        OCTVIS_TRACE("Input System Initialising!");

        if (s_System != nullptr) {
            OCTVIS_WARN("Input System already initialised.");
            return;
        } else {
            s_System = this;
        }
    }

    //############################################################################//
    // | EVENT PROCESSING |
    //############################################################################//

    void InputSystem::process_event(const SDL_Event& event) noexcept {

        switch (event.type) {
            // @off
            case SDL_MOUSEMOTION: {
                const SDL_MouseMotionEvent& mevent = event.motion;
                m_MousePosition.x    = mevent.x;
                m_MousePosition.y    = mevent.y;
                m_MousePosition.xrel = mevent.xrel;
                m_MousePosition.yrel = mevent.yrel;
                break;
            }

            case SDL_MOUSEWHEEL: {
                const SDL_MouseWheelEvent& wevent = event.wheel;
                m_MouseScroll.x  = wevent.x;
                m_MouseScroll.y  = wevent.y;
                m_MouseScroll.px = wevent.preciseX;
                m_MouseScroll.py = wevent.preciseY;
                break;
            }

            case SDL_KEYDOWN: {
                const SDL_KeyboardEvent& kevent = event.key;
                m_KeyState[kevent.keysym.sym] = KeyPressed{
                        kevent.keysym.sym,
                        kevent.keysym.mod
                };
                break;
            }
            case SDL_KEYUP: {
                const SDL_KeyboardEvent& kevent = event.key;
                m_KeyState[kevent.keysym.sym] = KeyReleased{
                        kevent.keysym.sym,
                        kevent.keysym.mod
                };
            }

            case SDL_MOUSEBUTTONDOWN: {
                const SDL_MouseButtonEvent& mevent = event.button;
                m_MouseButtonState[mevent.button] = MousePressed{
                        .button      = mevent.button,
                        .click_count = mevent.clicks,
                        .x           = mevent.x,
                        .y           = mevent.y
                };
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                const SDL_MouseButtonEvent& mevent = event.button;
                m_MouseButtonState[mevent.button] = MouseReleased{
                        .button      = mevent.button,
                        .click_count = mevent.clicks,
                        .x           = mevent.x,
                        .y           = mevent.y
                };
                break;
            }

            default:
                break;

            // @on
        }
    }

    void InputSystem::reset() noexcept {
        m_MouseScroll = { 0 };
        m_MousePosition.xrel = 0;
        m_MousePosition.yrel = 0;

        // Key Release Once
        for (auto it = m_KeyState.begin(); it != m_KeyState.end();) {
            if (!std::holds_alternative<KeyReleased>(it->second)) {
                ++it;
                continue;
            }
            it = m_KeyState.erase(it);
        }

        // Mouse Release Once
        for (auto it = m_MouseButtonState.begin(); it != m_MouseButtonState.end();) {
            if (!std::holds_alternative<MouseReleased>(it->second)) {
                ++it;
                continue;
            }
            it = m_MouseButtonState.erase(it);
        }
    }

    //############################################################################//
    // | STATIC API |
    //############################################################################//

    bool InputSystem::is_key_pressed(int key) noexcept {
        auto it = s_System->m_KeyState.find(key);
        return it != s_System->m_KeyState.end()
               && std::holds_alternative<KeyPressed>(it->second);
    }

    bool InputSystem::is_key_released(int key) noexcept {
        auto it = s_System->m_KeyState.find(key);
        return it != s_System->m_KeyState.end()
               && std::holds_alternative<KeyReleased>(it->second);
    }

    bool InputSystem::is_key_pressed(int key, std::initializer_list<int> mods) noexcept {
        auto it = s_System->m_KeyState.find(key);

        if (it == s_System->m_KeyState.end()) return false;
        if (!std::holds_alternative<KeyPressed>(it->second)) return false;

        KeyPressed pressed = std::get<KeyPressed>(it->second);

        // Build Mask
        for (int mod : mods) {
            if (!(pressed.mods & mod)) return false;
        }

        return true;
    }

    bool InputSystem::is_mouse_pressed(int btn) noexcept {
        auto it = s_System->m_MouseButtonState.find(btn);
        return it != s_System->m_MouseButtonState.end()
               && std::holds_alternative<MousePressed>(it->second);
    }

    bool InputSystem::is_mouse_released(int btn) noexcept {
        auto it = s_System->m_MouseButtonState.find(btn);
        return it != s_System->m_MouseButtonState.end()
               && std::holds_alternative<MouseReleased>(it->second);
    }

    glm::vec2 InputSystem::get_mouse_pos() noexcept {
        auto pos = s_System->m_MousePosition;
        return glm::vec2{ pos.x, pos.y };
    }

    glm::vec2 InputSystem::get_mouse_vel() noexcept {
        auto pos = s_System->m_MousePosition;
        return glm::vec2{ pos.xrel, pos.yrel };
    }

    glm::vec2 InputSystem::get_scroll_pos() noexcept {
        auto pos = s_System->m_MouseScroll;
        return glm::vec2{ pos.x, pos.y };
    }

    glm::vec2 InputSystem::get_scroll_vel() noexcept {
        auto pos = s_System->m_MouseScroll;
        return glm::vec2{ pos.px, pos.py };
    }

} // octvis