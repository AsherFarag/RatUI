find_package(SDL2 QUIET)

if(NOT SDL2_FOUND)
    include(FetchContent)
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
)

# ─── Examples ────────────────────────────────────────────────────────────────
add_subdirectory(SDL2)
