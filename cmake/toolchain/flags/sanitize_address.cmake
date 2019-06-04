# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

if (DEFINED POLLY_FLAGS_SANITIZE_ADDRESS_CMAKE_)
  return()
else ()
  set(POLLY_FLAGS_SANITIZE_ADDRESS_CMAKE_ 1)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/../../add_cache_flag.cmake)

add_cache_flag(CMAKE_CXX_FLAGS "-fsanitize=address")
add_cache_flag(CMAKE_CXX_FLAGS "-fsanitize-address-use-after-scope")
add_cache_flag(CMAKE_CXX_FLAGS "-g")

set(
    CMAKE_CXX_FLAGS_RELEASE
    "-O1 -DNDEBUG"
    CACHE
    STRING
    "C++ compiler flags"
    FORCE
)

add_cache_flag(CMAKE_C_FLAGS "-fsanitize=address")
add_cache_flag(CMAKE_C_FLAGS "-fsanitize-address-use-after-scope")
add_cache_flag(CMAKE_C_FLAGS "-g")

set(
    CMAKE_C_FLAGS_RELEASE
    "-O1 -DNDEBUG"
    CACHE
    STRING
    "C compiler flags"
    FORCE
)

set(ENV{ASAN_OPTIONS} detect_leaks=1)
