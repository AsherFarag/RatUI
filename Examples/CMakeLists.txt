# =============================================================================
# Examples
#
# The rendering/text dependencies already come in through the RatUI target via
# the enabled backends. The only extra thing the examples need is a window and
# input layer, which is SDL2.
# =============================================================================

ratui_require_dependency(
    NAME           SDL2
    TARGET         SDL2::SDL2
    FEATURE        "the RatUI examples"
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        ${RATUI_SDL2_TAG}
    CACHE_ARGS     SDL_TEST OFF
    ALIAS_FROM     SDL2
)

# Dependencies shared by every example.
add_library(RatUI_Examples_Common INTERFACE)

target_include_directories(RatUI_Examples_Common
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Common
)

target_link_libraries(RatUI_Examples_Common
    INTERFACE
        RatUI::RatUI
        SDL2::SDL2
)

# Copy SDL2 next to an example executable when it is built as a shared library.
get_target_property(RATUI_SDL2_TYPE SDL2::SDL2 TYPE)
if(RATUI_SDL2_TYPE STREQUAL "SHARED_LIBRARY")
    set(RATUI_SDL2_IS_SHARED ON)
else()
    set(RATUI_SDL2_IS_SHARED OFF)
endif()

function(ratui_copy_sdl2_runtime target)
    if(RATUI_SDL2_IS_SHARED)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:SDL2::SDL2>"
                "$<TARGET_FILE_DIR:${target}>"
            VERBATIM)
    endif()
endfunction()

add_subdirectory(SDL2)
