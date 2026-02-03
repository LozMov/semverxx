include(FetchContent)
FetchContent_Declare(argparse
        GIT_REPOSITORY https://github.com/p-ranav/argparse.git
        GIT_TAG v3.2
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        EXCLUDE_FROM_ALL
        FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(argparse)

add_executable(semv semv.cpp)
target_link_libraries(semv PRIVATE semverxx argparse)
target_compile_features(semv PRIVATE cxx_std_17)