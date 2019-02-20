add_library(hexutil
  hexutil.hpp
  hexutil.cpp
)
target_link_libraries(hexutil
  Boost::boost
  )

add_library(buffer
  buffer.hpp
  buffer.cpp
  )
target_link_libraries(buffer
  hexutil
  )
