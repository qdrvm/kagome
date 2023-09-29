
addtest(outcome_test
    outcome_test.cpp
    )
target_link_libraries(outcome_test
    outcome
    dummy_error
    )

addtest(formatters_test
    formatters_test.cpp
    )
target_link_libraries(formatters_test
    outcome
    soralog::soralog
    blob
    )

addtest(di_test di_test.cpp)


add_subdirectory(rocksdb)
