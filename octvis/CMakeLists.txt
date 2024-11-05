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
        public/Renderer.h
        public/Camera.h
        public/Application.h
        public/Context.h
        public/InputSystem.h
        public/RenderApplication.h
        public/WavefrontReader.h
        public/Utility.h
        public/Octree.h
        public/OctreeVisualisationApp.h
        public/PhysicsSystem.h
)

set(
        octvis_SOURCE_FILES

        public/Main.cpp
        private/Renderer.cpp
        private/Camera.cpp
        private/Context.cpp
        private/InputSystem.cpp
        private/RenderApplication.cpp
        private/WavefrontReader.cpp
        private/Utility.cpp
        private/Octree.cpp
        private/OctreeVisualisationApp.cpp
        private/PhysicsSystem.cpp
)

################################################################################
# Project Executable/Library
################################################################################

add_executable(
        octvis
        ${octvis_HEADER_FILES}
        ${octvis_SOURCE_FILES}
)

# Copies 'octvis/resources' into 'bin/resources' for relative pathing
add_custom_target(octvis_copy_resources ALL)

add_custom_command(
        TARGET octvis_copy_resources
        POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/octvis/resources"
        $<TARGET_FILE_DIR:octvis>/resources
        COMMENT "Copying resources/ into '$<TARGET_FILE_DIR:octvis>/resources'"
)

target_include_directories(${PROJECT_NAME} PUBLIC public PRIVATE private)

target_compile_definitions(${PROJECT_NAME} PUBLIC OCTVIS_ENABLE_ASSERTIONS)
target_compile_definitions(${PROJECT_NAME} PUBLIC OCTVIS_ENABLE_LOGGING)

target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm)
target_link_libraries(${PROJECT_NAME} PUBLIC GLEW::GLEW_static)

find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenGL::GL)

target_link_libraries(${PROJECT_NAME} PUBLIC SDL2::SDL2)
target_link_libraries(${PROJECT_NAME} PUBLIC imgui::imgui)
target_link_libraries(${PROJECT_NAME} PUBLIC stb_image)
target_link_libraries(${PROJECT_NAME} PUBLIC EnTT::EnTT)

################################################################################
# TESTS
################################################################################

add_subdirectory(tests)
