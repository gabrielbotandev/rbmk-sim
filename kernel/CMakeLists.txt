# C++ physics kernel: static library consumed by the orchestrator and tests.
add_library(rbmk_kernel STATIC
    src/core.cpp
    src/detectors.cpp
    src/kinetics.cpp
    src/rods.cpp
    src/thermal.cpp
    src/xenon_cpp.cpp)

target_include_directories(rbmk_kernel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rbmk_kernel
    PRIVATE rbmk_warnings
    PUBLIC rbmk_sanitizers)

if(RBMK_FORTRAN_AVAILABLE)
    target_compile_definitions(rbmk_kernel PRIVATE RBMK_HAVE_FORTRAN)
    target_link_libraries(rbmk_kernel PUBLIC rbmk_fortran)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
