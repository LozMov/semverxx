cmake_minimum_required(VERSION 3.23)
project(semverxx)

set(CMAKE_CXX_STANDARD 20)

add_library(semverxx INTERFACE)
target_sources(semverxx PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES include/semverxx.h
)

add_executable(demo example/demo.cpp)
target_link_libraries(demo semverxx)