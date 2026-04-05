find_package(SDL2 QUIET)
find_package(Freetype QUIET)
find_package(harfbuzz QUIET)

include(FetchContent)

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

# Interface library providing dependencies shared by all examples
add_library(RatUI_Examples_Common INTERFACE)

target_include_directories(RatUI_Examples_Common
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Common
)

target_link_libraries(RatUI_Examples_Common
    INTERFACE
        RatUI
        SDL2::SDL2
        Freetype::Freetype
        harfbuzz::harfbuzz
)

# ─── Examples ────────────────────────────────────────────────────────────────
add_subdirectory(SDL2)
