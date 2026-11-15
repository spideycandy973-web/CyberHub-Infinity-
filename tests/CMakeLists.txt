cmake_minimum_required(VERSION 3.22.1)
project(neon_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(core_tests
    core/test_pcg32.cpp
    ui/test_font_layout.cpp
)

target_include_directories(core_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/src/main/cpp
)

target_link_libraries(core_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(core_tests)
