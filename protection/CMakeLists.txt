# C11 reactor protection system (MISRA-inspired subset).
add_library(rbmk_rps STATIC src/rps.c)

target_include_directories(rbmk_rps PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rbmk_rps
    PRIVATE rbmk_warnings
    PUBLIC rbmk_sanitizers)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
