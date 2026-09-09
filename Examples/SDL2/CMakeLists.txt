add_executable(RatUI_Sandbox Main.cpp)

target_link_libraries(RatUI_Sandbox
    PRIVATE
        RatUI_Examples_Common
)

# Resources are loaded relative to the Examples/ directory.
set_target_properties(RatUI_Sandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

ratui_copy_sdl2_runtime(RatUI_Sandbox)
