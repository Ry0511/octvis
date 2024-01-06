//
// Date       : 03/01/2024
// Project    : octvis
// Author     : -Ry
//
#ifndef OCTVIS_INIT_H
#define OCTVIS_INIT_H

#include "Renderer.h"
#include "Logging.h"

//@off
#define SDL_MAIN_HANDLED
#include "SDL.h"
//@on

namespace octvis_tests {

    extern SDL_Window* window;
    extern SDL_GLContext context;

    extern float theta;
    extern float delta;

    using UserInitFunction = void* (*)();
    using UserdataFunction = void (*)(void*);
    using UserEventHandler = void (*)(void*, const SDL_Event&);

    extern UserInitFunction init_fn;
    extern UserEventHandler event_fn;
    extern UserdataFunction update_fn;
    extern UserdataFunction render_fn;
    extern UserdataFunction cleanup_fn;

    inline int init(const char* title) {

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
            OCTVIS_ERROR("Failed to initialise SDL; '{}'", SDL_GetError());
            return -1;
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

        // Create window with graphics context
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

        window = SDL_CreateWindow(
                title,
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                800, 600,
                SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL
        );

        context = SDL_GL_CreateContext(window);
        SDL_GL_MakeCurrent(window, context);
        SDL_GL_SetSwapInterval(1);

        if (glewInit() != GLEW_OK) {
            OCTVIS_ERROR("Failed to initialise GLEW");
            return -2;
        }

        return 0;
    }

    inline void set_functions(
            UserInitFunction init_fn_,
            UserEventHandler event_fn_,
            UserdataFunction update_fn_,
            UserdataFunction render_fn_,
            UserdataFunction cleanup_fn_
    ) {
        init_fn = init_fn_;
        event_fn = event_fn_;
        update_fn = update_fn_;
        render_fn = render_fn_;
        cleanup_fn = cleanup_fn_;
    }

    inline void cleanup() {
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
    }

    inline void start() {

        if (window == nullptr) init("Untitled Application");

        void* userdata = init_fn();
        size_t last_tick_count = SDL_GetTicks();

        bool is_running = true;
        while (is_running) {

            // GL Initialisation
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            int width, height;
            SDL_GetWindowSize(window, &width, &height);
            glViewport(0, 0, width, height);

            // Delta Timing
            size_t ticks_now = SDL_GetTicks();
            delta = (ticks_now - last_tick_count) * 0.001F;
            theta += delta;

            // Event Handling
            SDL_Event event{};
            while (SDL_PollEvent(&event)) {
                event_fn(userdata, event);
                if (event.type == SDL_QUIT) {
                    is_running = false;
                    break;
                }
            }

            // Update & Render
            update_fn(userdata);
            render_fn(userdata);
            SDL_GL_SwapWindow(window);
            last_tick_count = ticks_now;

        }

        cleanup_fn(userdata);
        cleanup();
    }

}

//@off
#ifdef OCTVIS_TESTS_INIT_VAR
    SDL_Window* octvis_tests::window = nullptr;
    SDL_GLContext octvis_tests::context = nullptr;

    float octvis_tests::theta = 0.0F;
    float octvis_tests::delta = 0.0F;

    void* (*octvis_tests::init_fn)()         = nullptr;
    void  (*octvis_tests::update_fn)(void*)  = nullptr;
    void  (*octvis_tests::render_fn)(void*)  = nullptr;
    void  (*octvis_tests::cleanup_fn)(void*) = nullptr;

    void  (*octvis_tests::event_fn)(void*, const SDL_Event&) = nullptr;

#endif
//@on

#endif
