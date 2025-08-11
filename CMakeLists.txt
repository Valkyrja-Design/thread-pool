cmake_minimum_required(VERSION 3.13)

project(ThreadPool)
set(CMAKE_CXX_STANDARD_REQUIRED 20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(ThreadPool)

target_compile_options(ThreadPool PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(ThreadPool PRIVATE -O3 -DNDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(ThreadPool PRIVATE -O0 -g)
endif()

add_subdirectory("include")
add_subdirectory(tests)

# Add custom test targets
enable_testing()

# Quick performance test
add_test(NAME quick_perf_test
    COMMAND ThreadPool --tasks 1000 --iterations 2
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Comprehensive performance test
add_test(NAME full_perf_test
    COMMAND ThreadPool --tasks 10000 --iterations 5
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Scalability test
add_test(NAME scalability_test
    COMMAND ThreadPool --tasks 5000 --threads 8 --iterations 3
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Stress test
add_custom_target(stress_test
    COMMAND ThreadPool --tasks 50000 --iterations 2
    DEPENDS ThreadPool
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running stress test with high task count"
)
