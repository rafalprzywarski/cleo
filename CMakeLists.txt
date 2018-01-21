project(cleo)
cmake_minimum_required(VERSION 2.8)

add_definitions("-std=c++14 -Wall -Wno-unused-const-variable")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

set(GoogleMock_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/libraries/GoogleMock/include")

add_subdirectory("source")
add_subdirectory("test")
add_subdirectory("libraries")
