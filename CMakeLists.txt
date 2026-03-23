cmake_minimum_required(VERSION 3.16)

project(RatUI VERSION 0.1 LANGUAGES CXX)

# =========================
# Options
# =========================
option(RATUI_BUILD_TESTS "Build RatUI tests" OFF)
option(RATUI_BUILD_EXAMPLES "Build RatUI examples" OFF)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =========================
# Header-only library
# =========================
add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:Include>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# =========================
# Subdirectories
# =========================
if(RATUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

if(RATUI_BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()
