# Orchestrator: couples kernel + RPS and exports the public C ABI as a
# shared library (loaded by the Python dashboard via ctypes).
add_library(rbmk_sim SHARED
    src/capi.cpp
    src/simulator.cpp)

target_include_directories(rbmk_sim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rbmk_sim
    PRIVATE rbmk_warnings rbmk_kernel rbmk_rps
    PUBLIC rbmk_sanitizers)
# The model version advertises whether the Fortran numerics are in the build,
# since run logs must identify exactly which numerics produced them.
set(RBMK_VERSION_SUFFIX "")

if(RBMK_FORTRAN_AVAILABLE)
    set(RBMK_VERSION_SUFFIX "+fortran")
endif()
target_compile_definitions(rbmk_sim
    PRIVATE RBMK_MODEL_VERSION="${PROJECT_VERSION}${RBMK_VERSION_SUFFIX}")
set_target_properties(rbmk_sim PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
