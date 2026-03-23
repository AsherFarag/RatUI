find_package(SDL2 REQUIRED)

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
add_subdirectory(Sandbox)
