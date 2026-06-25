# Coupled-system tests exercising the public C ABI end to end.
add_executable(rbmk_orchestrator_tests
    test_main.cpp
    test_capi.cpp
    test_coupling.cpp)

target_link_libraries(rbmk_orchestrator_tests
    PRIVATE rbmk_sim doctest::doctest rbmk_warnings rbmk_sanitizers)

add_test(NAME orchestrator.unit COMMAND rbmk_orchestrator_tests)
