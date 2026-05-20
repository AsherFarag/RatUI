find_package(SDL2 QUIET)
find_package(Freetype QUIET)
find_package(harfbuzz QUIET)

include(FetchContent)

set(RATUI_GLEW_IS_STATIC OFF)

if(NOT SDL2_FOUND)
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-2.30.9
    )
    FetchContent_MakeAvailable(SDL2)
    # Provide the SDL2::SDL2 target alias expected by the rest of this file
    if(NOT TARGET SDL2::SDL2)
        add_library(SDL2::SDL2 ALIAS SDL2)
    endif()
endif()

if(NOT Freetype_FOUND)
    FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
        GIT_TAG        VER-2-13-2
    )
    FetchContent_MakeAvailable(freetype)
    if(NOT TARGET Freetype::Freetype)
        add_library(Freetype::Freetype ALIAS freetype)
    endif()
endif()

if(NOT harfbuzz_FOUND)
    FetchContent_Declare(
        harfbuzz
        GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
        GIT_TAG        8.3.0
    )
    set(HB_BUILD_SUBSET OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(harfbuzz)
    if(NOT TARGET harfbuzz::harfbuzz)
        add_library(harfbuzz::harfbuzz ALIAS harfbuzz)
    endif()
endif()

find_package(msdfgen QUIET)
if(NOT msdfgen_FOUND)
    FetchContent_Declare(
        msdfgen
        GIT_REPOSITORY https://github.com/Chlumsky/msdfgen.git
        GIT_TAG        v1.12
    )
    set(MSDFGEN_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
    set(MSDFGEN_INSTALL OFF CACHE BOOL "" FORCE)
    set(MSDFGEN_USE_VCPKG OFF CACHE BOOL "" FORCE)
    set(MSDFGEN_DYNAMIC_RUNTIME ON CACHE BOOL "" FORCE)
    set(MSDFGEN_USE_SKIA OFF CACHE BOOL "" FORCE)
    set(MSDFGEN_DISABLE_SVG ON CACHE BOOL "" FORCE)
    set(MSDFGEN_DISABLE_PNG ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(msdfgen)
endif()

# ─── GLEW (OpenGL loader) ────────────────────────────────────────────────────
find_package(GLEW QUIET)

if(NOT GLEW_FOUND)
    FetchContent_Declare(
        glew
        URL https://downloads.sourceforge.net/project/glew/glew/2.2.0/glew-2.2.0.tgz
    )

    FetchContent_MakeAvailable(glew)

    # Manually create glew target
    add_library(glew STATIC
        ${glew_SOURCE_DIR}/src/glew.c
    )

    target_include_directories(glew PUBLIC
        ${glew_SOURCE_DIR}/include
    )

    target_compile_definitions(glew PRIVATE GLEW_STATIC)

    add_library(GLEW::GLEW ALIAS glew)
    set(RATUI_GLEW_IS_STATIC ON)
endif()

find_package(OpenGL REQUIRED)

# Interface library providing dependencies shared by all examples
add_library(RatUI_Examples_Common INTERFACE)

target_include_directories(RatUI_Examples_Common
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Common
)

if(RATUI_GLEW_IS_STATIC)
    target_compile_definitions(RatUI_Examples_Common
        INTERFACE
            GLEW_STATIC
    )
endif()

target_link_libraries(RatUI_Examples_Common
    INTERFACE
        RatUI
        SDL2::SDL2
        Freetype::Freetype
        harfbuzz::harfbuzz
        msdfgen::msdfgen
        GLEW::GLEW
        OpenGL::GL
)

# Build backend implementations into RatUI when examples/dependencies are enabled.
target_sources(RatUI
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source/RatUI/Backends/FreeType/FontCache.cpp
        ${CMAKE_SOURCE_DIR}/Source/RatUI/Backends/FreeType/TextUtil.cpp
        ${CMAKE_SOURCE_DIR}/Source/RatUI/Backends/FreeType/TextMetrics.cpp
        ${CMAKE_SOURCE_DIR}/Source/RatUI/Backends/OpenGL/OpenGLRenderer.cpp
)

target_link_libraries(RatUI
    PRIVATE
        Freetype::Freetype
        harfbuzz::harfbuzz
        msdfgen::msdfgen
        GLEW::GLEW
        OpenGL::GL
)

# ─── Examples ────────────────────────────────────────────────────────────────
add_subdirectory(SDL2)
