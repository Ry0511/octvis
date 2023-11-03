
#include <iostream>

#include "glm.hpp"

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

//#include "gl/GL.h"

int main() {

    glm::vec4 a{ 1.11, 2.2, 1.23, 0.0 };
    glm::vec4 b{ 1.25, 2.05, 1.1, 0.3 };
    glm::vec4 c = a * b;

    std::cout << "Initialisation\n";
    SDL_Init(SDL_INIT_EVERYTHING);
    ImGui::CreateContext();
    ImGui::DestroyContext();
    SDL_Quit();
    std::cout << "Termination\n";

    return 0;
}