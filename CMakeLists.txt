cmake_minimum_required(VERSION 3.25)

project(SentinelML
    VERSION 0.1.0
    DESCRIPTION "Custom inference engine for SentinelML"
    LANGUAGES CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. Use: cmake --preset release")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # for clangd / editor tooling

option(SENTINEL_BUILD_TESTS          "Build GoogleTest unit tests"                  ON)
option(SENTINEL_NATIVE_ARCH          "Tune for the host CPU (-mcpu=native)"          ON)
option(SENTINEL_SANITIZE             "Build with AddressSanitizer + UBSan"          OFF)
option(SENTINEL_WARNINGS_AS_ERRORS   "Treat compiler warnings as errors"            OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Must be called from the top-level directory so ctest (run from the build
# root) can see tests registered in subdirectories.
if(SENTINEL_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(cpp)
