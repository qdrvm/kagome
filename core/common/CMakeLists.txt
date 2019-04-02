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
  buffer_back_insert_iterator.cpp
  )

target_link_libraries(buffer
  hexutil
  )

add_library(blob
  blob.hpp
  blob.cpp
        ../../test/core/crypto/hasher/hasher_test.cpp)
target_link_libraries(blob
  hexutil
  )

add_library(logger
    logger.cpp
    )


