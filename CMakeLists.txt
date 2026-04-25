cmake_minimum_required(VERSION 3.14)
project(get_emails_attachement C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(pop3_core STATIC
    src/base64.c
    src/mime_parser.c
    src/pop3_client.c
)
target_include_directories(pop3_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(get_emails_attachement src/main.c)
target_link_libraries(get_emails_attachement PRIVATE pop3_core)

option(BUILD_TESTING "Build the test suite" ON)
if(BUILD_TESTING)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(pop3_tests
        tests/test_base64.cpp
        tests/test_mime_parser.cpp
        tests/test_pop3_client.cpp
    )
    target_link_libraries(pop3_tests PRIVATE pop3_core GTest::gtest_main pthread)

    include(GoogleTest)
    gtest_discover_tests(pop3_tests)
endif()
