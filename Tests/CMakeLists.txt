ratui_require_dependency(
    NAME           Catch2
    TARGET         Catch2::Catch2WithMain
    FEATURE        "the RatUI test suite"
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        ${RATUI_CATCH2_TAG}
)

file(GLOB_RECURSE RATUI_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)
list(FILTER RATUI_TEST_SOURCES EXCLUDE REGEX "/Package/") # Standalone find_package() consumer.

add_executable(RatUI_Tests ${RATUI_TEST_SOURCES})

target_include_directories(RatUI_Tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(RatUI_Tests
    PRIVATE
        RatUI::RatUI
        Catch2::Catch2WithMain
)

# Font tests load the example fonts when the FreeType backend is enabled.
target_compile_definitions(RatUI_Tests
    PRIVATE
        RATUI_TEST_FONTS_DIR="${PROJECT_SOURCE_DIR}/Examples/Resources/Fonts"
)

# The fetched Catch2 does not add its helper scripts to the module path itself.
if(DEFINED Catch2_SOURCE_DIR AND EXISTS "${Catch2_SOURCE_DIR}/extras/Catch.cmake")
    list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
endif()

include(Catch)
catch_discover_tests(RatUI_Tests)
