add_subdirectory(binaryen)

addtest(outcome_test outcome_test.cpp)
target_link_libraries(outcome_test
  outcome
  )


addtest(sml_test sml_test.cpp)

addtest(leveldb_test leveldb_test.cpp)
target_link_libraries(leveldb_test
  leveldb::leveldb
  Boost::boost
  Boost::filesystem
  )
