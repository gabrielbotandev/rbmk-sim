# RBMK-SIM superbuild: C++ kernel, C protection system, orchestrator (+ C ABI),
# optional Fortran numerics, and all native tests.
#
# Native workflow (no wrapper scripts):
#   cmake --preset dev && cmake --build --preset dev && ctest --preset dev
cmake_minimum_required(VERSION 3.24)

project(rbmk_sim
    VERSION 0.1.0
    DESCRIPTION "Educational RBMK-1000-inspired reactor simulator (toy model)"
    LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(RBMK_ENABLE_SANITIZERS "Build native tests with ASan/UBSan" OFF)
option(RBMK_ENABLE_CLANG_TIDY "Run clang-tidy as part of compilation" OFF)
option(RBMK_ENABLE_FORTRAN "Build the Fortran numerics (auto-detected)" ON)

# Common warning profile, applied to every first-party target.
add_library(rbmk_warnings INTERFACE)
target_compile_options(rbmk_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
    -Wdouble-promotion -Wformat=2 -Wundef -Wcast-align
    $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes -Wmissing-prototypes>
    $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor -Wold-style-cast>)

add_library(rbmk_sanitizers INTERFACE)

if(RBMK_ENABLE_SANITIZERS)
    target_compile_options(rbmk_sanitizers INTERFACE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(rbmk_sanitizers INTERFACE -fsanitize=address,undefined)
endif()

if(RBMK_ENABLE_CLANG_TIDY)
    find_program(RBMK_CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
    set(CMAKE_C_CLANG_TIDY "${RBMK_CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY "${RBMK_CLANG_TIDY_EXE}")
endif()

# Optional Fortran numerics: auto-detected; the kernel falls back to its C++
# twin implementation when no Fortran compiler is installed
# (Fedora: sudo dnf install gcc-gfortran).
set(RBMK_FORTRAN_AVAILABLE FALSE)

if(RBMK_ENABLE_FORTRAN)
    include(CheckLanguage)
    check_language(Fortran)
    if(CMAKE_Fortran_COMPILER)
        enable_language(Fortran)
        set(RBMK_FORTRAN_AVAILABLE TRUE)
        message(STATUS "RBMK: Fortran numerics enabled (${CMAKE_Fortran_COMPILER})")
    else()
        message(STATUS
            "RBMK: no Fortran compiler found - using the C++ fallback numerics "
            "(install gcc-gfortran and reconfigure to enable)")
    endif()
endif()

include(CTest)
enable_testing()

if(BUILD_TESTING)
    # Test frameworks, fetched once at pinned tags (SYSTEM silences their
    # warnings under our strict profile).
    include(FetchContent)
    FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.12
        SYSTEM)
    FetchContent_Declare(unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG v2.6.1
        SYSTEM)
    FetchContent_MakeAvailable(doctest unity)
endif()

if(RBMK_FORTRAN_AVAILABLE)
    add_subdirectory(fortran)
endif()
add_subdirectory(kernel)
add_subdirectory(protection)
add_subdirectory(orchestrator)
