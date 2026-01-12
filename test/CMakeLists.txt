find_package(GTest)

if (NOT GTest_FOUND)
    message(STATUS "Fetching googletest...")
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
    )
    # Windows: Use shared (DLL) run-time lib even when Google Test is built as static lib
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif ()

add_executable(semverxx_test semverxx_test.cpp)
target_link_libraries(semverxx_test PRIVATE semverxx GTest::gtest_main)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(semverxx_test PRIVATE -Wall -Wextra -pedantic-errors -Werror)
endif ()

include(GoogleTest)
gtest_discover_tests(semverxx_test)
