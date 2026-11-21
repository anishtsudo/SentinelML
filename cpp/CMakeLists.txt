include(CheckCXXCompilerFlag)

# ---------------------------------------------------------------------------
# Shared compile settings, applied to every target via an INTERFACE library
# so flags are declared once instead of copy-pasted per target.
# ---------------------------------------------------------------------------
add_library(sentinel_options INTERFACE)
target_compile_features(sentinel_options INTERFACE cxx_std_20)
target_compile_options(sentinel_options INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)

if(SENTINEL_WARNINGS_AS_ERRORS)
    target_compile_options(sentinel_options INTERFACE -Werror)
endif()

if(SENTINEL_NATIVE_ARCH)
    check_cxx_compiler_flag("-mcpu=native" SENTINEL_HAS_MCPU_NATIVE)
    if(SENTINEL_HAS_MCPU_NATIVE)
        target_compile_options(sentinel_options INTERFACE -mcpu=native)
    endif()
endif()

if(SENTINEL_SANITIZE)
    target_compile_options(sentinel_options INTERFACE
        -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    target_link_options(sentinel_options INTERFACE -fsanitize=address,undefined)
endif()

# ---------------------------------------------------------------------------
# Core library (the inference engine will grow here)
# ---------------------------------------------------------------------------
add_library(sentinel_core
    src/build_info.cpp
    src/simd.cpp)
add_library(sentinel::core ALIAS sentinel_core)

target_include_directories(sentinel_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(sentinel_core PUBLIC sentinel_options)
target_compile_definitions(sentinel_core PRIVATE
    SENTINEL_BUILD_TYPE="$<CONFIG>"
    SENTINEL_VERSION="${PROJECT_VERSION}")

# ---------------------------------------------------------------------------
# Tools
# ---------------------------------------------------------------------------
add_executable(sentinel_info tools/sentinel_info.cpp)
target_link_libraries(sentinel_info PRIVATE sentinel::core)

# ---------------------------------------------------------------------------
# Tests (GoogleTest fetched at a pinned version for reproducibility)
# ---------------------------------------------------------------------------
if(SENTINEL_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SYSTEM)  # third-party headers: don't apply our -W flags to them
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # clang 21 added -Wcharacter-conversion, which fires inside GoogleTest 1.15.2's
    # own sources. Silence it for that target only; our code keeps the warning.
    check_cxx_compiler_flag("-Wno-character-conversion" SENTINEL_HAS_WNO_CHAR_CONV)
    if(SENTINEL_HAS_WNO_CHAR_CONV)
        target_compile_options(gtest PRIVATE -Wno-character-conversion)
    endif()

    include(GoogleTest)

    add_executable(sentinel_tests
        tests/test_build_info.cpp
        tests/test_simd.cpp)
    target_link_libraries(sentinel_tests PRIVATE sentinel::core GTest::gtest_main)
    gtest_discover_tests(sentinel_tests)
endif()
