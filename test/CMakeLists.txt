include(CTest)

set(UNIT_TESTS
    test_task_manager
    test_task_ranking_parameter
    test_task_execution_ranking
)

foreach(TEST ${UNIT_TESTS})
    add_executable(${TEST} ${TEST}.cpp)
    target_link_libraries(${TEST} pexplore)
    add_test(${TEST} ${TEST})
endforeach()

add_custom_target(tests)
add_dependencies(tests ${UNIT_TESTS})
