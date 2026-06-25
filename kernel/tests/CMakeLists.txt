# Kernel unit tests (doctest is provided by the top-level CMakeLists).
add_executable(rbmk_kernel_tests
    test_main.cpp
    test_determinism.cpp
    test_kinetics.cpp
    test_rods.cpp
    test_scram.cpp
    test_thermal.cpp
    test_xenon.cpp)

target_link_libraries(rbmk_kernel_tests
    PRIVATE rbmk_kernel doctest::doctest rbmk_warnings rbmk_sanitizers)

add_test(NAME kernel.unit COMMAND rbmk_kernel_tests)
