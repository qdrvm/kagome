
addtest(outcome_test outcome_test.cpp)
target_link_libraries(outcome_test
    outcome
    )

addtest(di_test di_test.cpp)

add_subdirectory(binaryen)
add_subdirectory(wavm)
add_subdirectory(rocksdb)
