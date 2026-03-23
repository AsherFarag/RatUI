include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.0
)

FetchContent_MakeAvailable(Catch2)

add_executable(RatUI_Tests
    Core/TestVec.cpp
    Core/TestContainers.cpp
    Layout/TestLayout.cpp
    StringFormat/TestStringFormat.cpp
)

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
