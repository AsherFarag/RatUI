find_package(SDL2 QUIET)
find_package(SDL2_ttf QUIET)

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

if(NOT SDL2_ttf_FOUND)
    FetchContent_Declare(
        SDL2_ttf
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG        release-2.22.0
    )
    FetchContent_MakeAvailable(SDL2_ttf)
    # Provide the SDL2_ttf::SDL2_ttf target alias expected by the rest of this file
    if(NOT TARGET SDL2_ttf::SDL2_ttf)
        add_library(SDL2_ttf::SDL2_ttf ALIAS SDL2_ttf)
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
        SDL2_ttf::SDL2_ttf
)

# ─── Examples ────────────────────────────────────────────────────────────────
add_subdirectory(SDL2)
