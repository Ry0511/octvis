cmake_minimum_required(VERSION 3.23)
project(octvis VERSION 0.0.0 DESCRIPTION "CSP400 Masters Project")
set(CMAKE_CXX_STANDARD 20)

################################################################################
# Header Files & Source Files
################################################################################

set(PROJECT_NAME "octvis")

set(
        octvis_HEADER_FILES
        public/Logging.h # Could be in private
)

set(
        octvis_SOURCE_FILES
        public/Main.cpp
)

################################################################################
# Project Executable/Library
################################################################################

add_executable(
        octvis
        ${octvis_HEADER_FILES}
        ${octvis_SOURCE_FILES}
)

target_include_directories(${PROJECT_NAME} PUBLIC public PRIVATE private)

target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
target_link_libraries(${PROJECT_NAME} PRIVATE GLEW::GLEW)
target_link_libraries(${PROJECT_NAME} PRIVATE octvis_opengl)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 PRIVATE SDL2::SDL2main)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui)