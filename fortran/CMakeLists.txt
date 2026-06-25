# Optional Fortran numerics. Only added by the superbuild when a Fortran
# compiler was detected (see the top-level CMakeLists.txt); otherwise the
# kernel uses its C++ fallback implementation of the same algorithms.
add_library(rbmk_fortran STATIC src/xenon_kinetics.f90)

set_target_properties(rbmk_fortran PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(rbmk_fortran PUBLIC rbmk_sanitizers)
