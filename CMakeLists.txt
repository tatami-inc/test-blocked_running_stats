cmake_minimum_required(VERSION 3.14)

project(summation
    VERSION 1.0.0
    DESCRIPTION "Performance tests for summation"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11
  GIT_TAG v2.4.1
)

FetchContent_Declare(
    nanobench
    GIT_REPOSITORY https://github.com/martinus/nanobench
    GIT_TAG v4.3.11
)

FetchContent_MakeAvailable(cli11)
FetchContent_MakeAvailable(nanobench)

add_executable(dense_sum dense_sum.cpp)
target_link_libraries(dense_sum nanobench::nanobench CLI11::CLI11)

add_executable(dense_var dense_var.cpp)
target_link_libraries(dense_var nanobench::nanobench CLI11::CLI11)

add_executable(sparse_sum sparse_sum.cpp)
target_link_libraries(sparse_sum nanobench::nanobench CLI11::CLI11)

