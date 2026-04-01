add_executable(RatUI_Sandbox Main.cpp)

add_custom_command(TARGET RatUI_Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:SDL2::SDL2>"
        $<TARGET_FILE_DIR:RatUI_Sandbox>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different       
        "$<TARGET_FILE:SDL2_ttf::SDL2_ttf>"
        $<TARGET_FILE_DIR:RatUI_Sandbox>)

target_link_libraries(RatUI_Sandbox
    PRIVATE
        RatUI_Examples_Common
)
