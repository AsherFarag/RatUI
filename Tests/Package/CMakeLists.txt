# Builds against an installed RatUI to check the exported package works.
#   cmake -B build -S Tests/Package -DCMAKE_PREFIX_PATH=<RatUI install prefix>
cmake_minimum_required(VERSION 3.16)
project(RatUIPackageTest LANGUAGES CXX)

find_package(RatUI 0.1 REQUIRED)

add_executable(RatUIPackageTest Main.cpp)
target_link_libraries(RatUIPackageTest PRIVATE RatUI::RatUI)

enable_testing()
add_test(NAME RatUIPackageTest COMMAND RatUIPackageTest)
