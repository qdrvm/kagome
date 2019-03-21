add_subdirectory(binaryen)

addtest(outcome_test outcome_test.cpp)
target_link_libraries(outcome_test
  outcome
  )
