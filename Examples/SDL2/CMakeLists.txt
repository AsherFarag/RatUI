add_executable(RatUI_Sandbox Main.cpp)

# Set the working directory to the Examples folder
set_target_properties(RatUI_Sandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../"
)

add_custom_command(TARGET RatUI_Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:SDL2::SDL2>"
        $<TARGET_FILE_DIR:RatUI_Sandbox>)

target_link_libraries(RatUI_Sandbox
    PRIVATE
        RatUI_Examples_Common
)
