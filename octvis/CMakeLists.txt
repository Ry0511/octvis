cmake_minimum_required(VERSION 3.23)
project(octvis VERSION 0.0.0 DESCRIPTION "CSP400 Masters Project")
set(CMAKE_CXX_STANDARD 20)

################################################################################
# Header Files & Source Files
################################################################################

set(PROJECT_NAME "octvis")

set(
        octvis_HEADER_FILES
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