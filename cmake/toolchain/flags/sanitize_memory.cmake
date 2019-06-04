# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

if (DEFINED POLLY_FLAGS_SANITIZE_MEMORY_CMAKE_)
  return()
else ()
  set(POLLY_FLAGS_SANITIZE_MEMORY_CMAKE_ 1)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/../../add_cache_flag.cmake)

add_cache_flag(CMAKE_CXX_FLAGS "-fsanitize=memory")
add_cache_flag(CMAKE_CXX_FLAGS "-fsanitize-memory-track-origins")
add_cache_flag(CMAKE_CXX_FLAGS "-g")

add_cache_flag(CMAKE_C_FLAGS "-fsanitize=memory")
add_cache_flag(CMAKE_C_FLAGS "-fsanitize-memory-track-origins")
add_cache_flag(CMAKE_C_FLAGS "-g")
