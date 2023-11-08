cmake_minimum_required(VERSION 3.26)
project(octvis)

set(CMAKE_CXX_STANDARD 20)

################################################################################
# Output Directories & Windows Symbols
################################################################################

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

################################################################################
# SUB DIRECTORIES
################################################################################

add_subdirectory(vendor)
add_subdirectory(octvis)
