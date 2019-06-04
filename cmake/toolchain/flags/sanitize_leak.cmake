# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

if (DEFINED POLLY_FLAGS_SANITIZE_LEAK_CMAKE_)
  return()
else ()
  set(POLLY_FLAGS_SANITIZE_LEAK_CMAKE_ 1)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/../../add_cache_flag.cmake)

add_cache_flag(CMAKE_CXX_FLAGS "-fsanitize=leak")
add_cache_flag(CMAKE_CXX_FLAGS "-g")

add_cache_flag(CMAKE_C_FLAGS "-fsanitize=leak")
add_cache_flag(CMAKE_C_FLAGS "-g")

set(ENV{LSAN_OPTIONS} detect_leaks=1)

list(APPEND HUNTER_TOOLCHAIN_UNDETECTABLE_ID "sanitize-leak")
