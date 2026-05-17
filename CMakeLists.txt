cmake_minimum_required(VERSION 3.20)
project(resource_pool_assignment LANGUAGES CXX)

# Candidates: set this to 17 or 20 based on the standard you choose to target.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Dependencies ─────────────────────────────────────────────────────────────

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ── Tests ─────────────────────────────────────────────────────────────────────

enable_testing()

add_executable(resource_pool_tests
    tests/test_resource_pool.cpp
)

target_include_directories(resource_pool_tests PRIVATE include)

#Adding warning flags for MSVC compilers
if (MSVC)
    target_compile_options(resource_pool_tests PRIVATE /W4 /WX)
else()
    target_compile_options(resource_pool_tests PRIVATE -Wall -Wextra -Werror)
endif()

target_link_libraries(resource_pool_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(resource_pool_tests)
