add_subdirectory(binaryen)

addtest(outcome_test outcome_test.cpp)
target_link_libraries(outcome_test
    outcome
    )


addtest(sml_test sml_test.cpp)

addtest(continuable_test continuable_test.cpp)
