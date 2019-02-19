addtest(common_test
  hexutil_test.cpp
  buffer_test.cpp
  )
target_link_libraries(common_test
  hexutil
  buffer
  )

