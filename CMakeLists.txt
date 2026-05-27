cmake_minimum_required(VERSION 3.16)

project(RatUI VERSION 0.1 LANGUAGES CXX)

# =========================
# Options
# =========================
option(RATUI_BUILD_TESTS "Build RatUI tests" OFF)
option(RATUI_BUILD_EXAMPLES "Build RatUI examples" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =========================
# Library
# =========================
file(GLOB_RECURSE RATUI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)

add_library(${PROJECT_NAME}
    ${RATUI_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:Include>
)

target_compile_features(RatUI PUBLIC cxx_std_20)

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
