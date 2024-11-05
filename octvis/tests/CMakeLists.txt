cmake_minimum_required(VERSION 3.23)
project(octvis_tests VERSION 1.0.0 DESCRIPTION "Test suite for octvis")
set(CMAKE_CXX_STANDARD 20)

# Header Files & Source Files

set(PROJECT_NAME "octvis_tests")

################################################################################
# MAIN LIBRARY TO TEST
################################################################################

add_library(${PROJECT_NAME} STATIC)

# Get the 'octvis' sources and turn then into absolute paths
get_target_property(OCTVIS_SOURCES octvis SOURCES)
set(ABSOLUTE_OCTVIS_SOURCES "")
foreach (RELATIVE_PATH ${OCTVIS_SOURCES})
    if (RELATIVE_PATH MATCHES ".*Main\.cpp")
        continue()
    endif ()

    get_filename_component(
            ABSOLUTE_PATH "${RELATIVE_PATH}"
            ABSOLUTE BASE_DIR "../"
    )
    list(APPEND ABSOLUTE_OCTVIS_SOURCES "${ABSOLUTE_PATH}")
endforeach ()

# Attach Absolute sources
target_sources(${PROJECT_NAME} PRIVATE ${ABSOLUTE_OCTVIS_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ../public PRIVATE ../private)

target_compile_definitions(${PROJECT_NAME} PUBLIC OCTVIS_ENABLE_ASSERTIONS)
target_compile_definitions(${PROJECT_NAME} PUBLIC OCTVIS_ENABLE_LOGGING)

target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm)
target_link_libraries(${PROJECT_NAME} PUBLIC GLEW::GLEW_static)

find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenGL::GL)

target_link_libraries(${PROJECT_NAME} PUBLIC SDL2::SDL2)
target_link_libraries(${PROJECT_NAME} PUBLIC imgui::imgui)
target_link_libraries(${PROJECT_NAME} PUBLIC stb_image)

set_target_properties(${PROJECT_NAME}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

################################################################################
# GENERATE TESTS
################################################################################

set(test_index 0)
function(create_visual_test test_name test_sources)

    add_executable(${test_name} ${test_sources})
    target_link_libraries(${test_name} PRIVATE ${PROJECT_NAME})

    set_target_properties(${test_name}
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests/visual"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests/visual"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests/visual"
    )

    # Copy Resources
    add_custom_command(
            TARGET ${test_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/visual/resources"
            "$<TARGET_FILE_DIR:${test_name}>/resources"
    )

    # Copy SDL2d.dll into the tests directory
    add_custom_command(
            TARGET ${test_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/bin/SDL2d.dll"
            "${CMAKE_BINARY_DIR}/bin/tests/visual"
    )

    add_test(NAME ${test_name}_${test_index} COMMAND ${test_name})

    math(EXPR test_index "${test_index} + 1")
endfunction()

create_visual_test(triangle_visual_test "visual/triangle_render.cpp")