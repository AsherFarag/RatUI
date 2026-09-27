# =============================================================================
# Examples
#
# One executable that runs an Example (see Main.cpp) in an SDL2 window. The
# rendering and text backends come in through the RatUI target; SDL2 is the
# only extra dependency.
# =============================================================================

if(EMSCRIPTEN)
    # Emscripten ships SDL2 as a built-in port.
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    target_compile_options(SDL2::SDL2 INTERFACE -sUSE_SDL=2)
    target_link_options(SDL2::SDL2 INTERFACE -sUSE_SDL=2)
endif()

ratui_require_dependency(
    NAME           SDL2
    TARGET         SDL2::SDL2
    FEATURE        "the RatUI examples"
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        ${RATUI_SDL2_TAG}
    CACHE_ARGS     SDL_TEST OFF
    ALIAS_FROM     SDL2
)

add_executable(RatUI_Examples
    Main.cpp
    Application/Application.cpp
)

target_link_libraries(RatUI_Examples PRIVATE RatUI::RatUI SDL2::SDL2)

# Fonts are loaded from Resources/, relative to this directory.
set_target_properties(RatUI_Examples PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# A shared SDL2 has to sit next to the executable.
get_target_property(RATUI_SDL2_TYPE SDL2::SDL2 TYPE)
if(RATUI_SDL2_TYPE STREQUAL "SHARED_LIBRARY")
    add_custom_command(TARGET RatUI_Examples POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:SDL2::SDL2>" "$<TARGET_FILE_DIR:RatUI_Examples>"
        VERBATIM)
endif()

# Browser build: emit an HTML page and pack Resources/ into its virtual file system.
if(EMSCRIPTEN)
    set(RATUI_WEB_SHELL "${CMAKE_CURRENT_SOURCE_DIR}/Application/Shell.html")
    set_target_properties(RatUI_Examples PROPERTIES
        SUFFIX       ".html"
        LINK_DEPENDS "${RATUI_WEB_SHELL}"
    )
    target_link_options(RatUI_Examples PRIVATE
        -sALLOW_MEMORY_GROWTH=1
        "--shell-file=${RATUI_WEB_SHELL}"
        "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/Resources@/Resources"
    )
endif()
