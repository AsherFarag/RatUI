include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.0
)

FetchContent_MakeAvailable(Catch2)

add_executable(RatUI_Tests
    test_main.cpp
)

target_link_libraries(RatUI_Tests
    PRIVATE
        RatUI
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(RatUI_Tests)