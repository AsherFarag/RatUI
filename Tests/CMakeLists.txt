include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.0
)

FetchContent_MakeAvailable(Catch2)

# Gather all test source files - Note all .cpp files in the directory are Tests, so we can use GLOB_RECURSE to find them
file(GLOB_RECURSE TEST_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_executable(RatUI_Tests ${TEST_SOURCES})

target_include_directories(RatUI_Tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(RatUI_Tests
    PRIVATE
        RatUI
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(RatUI_Tests)
