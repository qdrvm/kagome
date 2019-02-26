add_library(twox twox.cpp)
target_link_libraries(twox
  xxhash
  buffer
  )
