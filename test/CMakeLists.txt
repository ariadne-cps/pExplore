include(CTest)

set(UNIT_TESTS
    test_task_manager
    test_task_ranking_constraint
    test_task_execution_ranking
    test_task_runner
)

foreach(TEST ${UNIT_TESTS})
    add_executable(${TEST} ${TEST}.cpp)
    target_link_libraries(${TEST} pexplore)
    add_test(${TEST} ${TEST})
endforeach()

add_custom_target(tests)
add_dependencies(tests ${UNIT_TESTS})
