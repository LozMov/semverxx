cmake_minimum_required(VERSION 3.23)
project(semverxx LANGUAGES CXX)

option(SEMVERXX_BUILD_CLI "Build the version parser CLI program" OFF)

add_library(semverxx INTERFACE)
target_sources(semverxx PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES include/semverxx.h
)

if (PROJECT_IS_TOP_LEVEL)
    include(CTest)
    if (BUILD_TESTING)
        add_subdirectory(test)
    endif ()
endif ()