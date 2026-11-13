cmake_minimum_required(VERSION 3.10)

project(pfodParser LANGUAGES CXX C)

# Plain C++ port of the pfodParser core library (parsing + drawing commands).
# Requires C++11 or later; no Arduino toolchain needed.
# Tested with: GCC >= 4.8, Clang >= 3.3.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB PFOD_PARSER_CXX_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/dwgs/*.cpp"
)
file(GLOB PFOD_PARSER_C_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)
# NOTE: file(GLOB ...) only re-scans when CMake is re-run (e.g. after editing
# CMakeLists.txt or a fresh build-dir configure). If you add or remove a
# source file, re-run cmake so the new file list is picked up.

add_library(pfodParser STATIC
    ${PFOD_PARSER_CXX_SOURCES}
    ${PFOD_PARSER_C_SOURCES}
)

target_include_directories(pfodParser PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(pfodParser PUBLIC cxx_std_11)

target_compile_options(pfodParser PRIVATE -Wall -Wextra)

install(TARGETS pfodParser
    ARCHIVE DESTINATION lib
)
install(DIRECTORY src/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
