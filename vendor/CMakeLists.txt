cmake_minimum_required(VERSION 3.23)
project(vendor_libs VERSION 0.0.0 DESCRIPTION "")
set(CMAKE_CXX_STANDARD 20)

# - NOTE -
# If not using vendor libraries it assumes that vcpkg conventions are used. It is best to avoid
# using a package manager if you do not want to modify the CMake files to get things working.
#

################################################################################
# OPEN GL
################################################################################

# - NOTE -
# OpenGL is system dependent and should be provided by the OS
#
# - NOTE -
# This is just to expose a custom opengl target for convenience and single point management.
#

find_package(OpenGL REQUIRED)
add_library(octvis_opengl INTERFACE)
target_link_libraries(octvis_opengl INTERFACE OpenGL::GL)

################################################################################
# GLEW
################################################################################

option(OCTVIS_USE_VENDOR_GLEW "Use the vendor glew target" ON)

if (NOT OCTVIS_USE_VENDOR_GLEW)
    find_package(GLEW REQUIRED)
    # target_link_libraries(main PRIVATE GLEW::GLEW)

    if (NOT TARGET GLEW::GLEW)
        message(FATAL_ERROR "Found GLEW but the target GLEW::GLEW was not found...")
    endif ()

else ()
    add_subdirectory(glew)

    set_target_properties( # We want GLEW to build into our output directories
            libglew_shared libglew_static
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lib
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    add_library(GLEW::GLEW ALIAS libglew_shared)
    add_library(GLEW::GLEW_static ALIAS libglew_static)

endif ()

################################################################################
# GLM CMAKE TARGET
################################################################################

# - SOURCE -
# Vendor Link: https://github.com/Groovounet/glm

# Since GLM is a header only library we have to interface.
# We only need to know where the glm header files are.

# Note: Warning C5033 exists in GLM when compiling with later versions.

option(OCTVIS_USE_VENDOR_GLM "Use the vendored glm target" ON)

if (NOT OCTVIS_USE_VENDOR_GLM)
    message(STATUS "Using Package Manager GLM")
    find_package(glm CONFIG REQUIRED)

else ()

    if (NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glm/glm)
        message(FATAL_ERROR "Failed to find glm at '${CMAKE_CURRENT_SOURCE_DIR}/glm/glm'")
    endif ()

    add_library(glm INTERFACE)

    target_include_directories(
            glm
            INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/glm/glm>
            $<INSTALL_INTERFACE:include>
    )

    # Lets you link to glm using both 'glm' and 'glm::glm' targets
    add_library(glm::glm ALIAS glm)

endif ()

################################################################################
# SDL2
################################################################################

option(OCTVIS_USE_VENDOR_SDL2 "Use the vendored glm target" ON)

if (NOT ${OCTVIS_USE_VENDOR_SDL2})
    find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
    find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)

else ()

    # - NOTE -
    message(TRACE "You may need to build SDL2 for the headers to be available at 'SDL2/*.h'")

    if (NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sdl2)
        message(FATAL_ERROR "Can't find sdl2 at '${CMAKE_CURRENT_SOURCE_DIR}/sdl2'")
    endif ()

    add_subdirectory(sdl2 EXCLUDE_FROM_ALL)
    if (NOT TARGET SDL2::SDL2 OR NOT TARGET SDL2::SDL2main)
        message(FATAL_ERROR "Failed to find SDL2 CMake targets 'SDL2::SDL2' and SDL2::SDL2main")
    endif ()

endif ()

################################################################################
# IMGUI
################################################################################

option(OCTVIS_USE_VENDOR_IMGUI "Use the vendored imgui target..." ON)

if (NOT ${OCTVIS_USE_VENDOR_IMGUI})
    find_package(imgui CONFIG REQUIRED)
    # target_link_libraries(main PRIVATE imgui::imgui)

else ()

    if (NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
        message(FATAL_ERROR "Failed to find imgui sources at '${CMAKE_CURRENT_SOURCE_DIR}/imgui'")
    endif ()

    # You can link to imgui with a shared library however it requires extra attention when
    #  communicating through DLL boundaries. This is not something we care about here though.

    file( # GLOB STANDARD IMGUI HEADERS & SOURCE FILES
            GLOB
            OCTVIS_IMGUI_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.h"
    )

    list( # APPEND IMGUI SDL2 & OPENGL3 BACKENDS
            APPEND
            OCTVIS_IMGUI_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_sdl2.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_sdl2.cpp"
    )

    add_library(imgui STATIC ${OCTVIS_IMGUI_SOURCES})

    target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
    target_link_libraries(imgui PRIVATE SDL2::SDL2 PRIVATE SDL2::SDL2main)
    target_link_libraries(imgui PRIVATE octvis_opengl)

    # Allow linking with 'imgui' and 'imgui::imgui'
    add_library(imgui::imgui ALIAS imgui)

endif ()