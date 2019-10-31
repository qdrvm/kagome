add_subdirectory(binaryen)

addtest(outcome_test outcome_test.cpp)
target_link_libraries(outcome_test
    Boost::boost
    )

addtest(sml_test sml_test.cpp)


addtest(di_test di_test.cpp)
