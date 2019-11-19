project(cleo)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS_FASTDEBUG "-g -O2" CACHE STRING "C++ flags for FastDebug builds" FORCE)
set(CMAKE_C_FLAGS_FASTDEBUG "-g -O2" CACHE STRING "C flags for FastDebug builds" FORCE)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;MinSizeRel;RelWithDebInfo;FastDebug")
mark_as_advanced(CMAKE_CXX_FLAGS_FASTDEBUG CMAKE_C_FLAGS_FASTDEBUG CMAKE_CONFIGURATION_TYPES)

add_definitions("-std=c++14 -Wall -Wsign-compare")
if(APPLE)
	add_definitions("-Wno-unused-const-variable")
else(APPLE)
        add_definitions("-Wno-psabi")
endif(APPLE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

set(GoogleMock_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/libraries/GoogleMock/include")

add_subdirectory("source")
add_subdirectory("test")
add_subdirectory("libraries")
