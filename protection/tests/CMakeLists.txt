# RPS unit tests (Unity is provided by the top-level CMakeLists).
add_executable(rbmk_rps_tests test_rps.c)
target_link_libraries(rbmk_rps_tests PRIVATE rbmk_rps unity rbmk_warnings rbmk_sanitizers)

add_test(NAME rps.unit COMMAND rbmk_rps_tests)
