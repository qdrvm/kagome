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
add_cache_flag(CMAKE_CXX_FLAGS "-O1")
add_cache_flag(CMAKE_CXX_FLAGS "-DNDEBUG")

add_cache_flag(CMAKE_C_FLAGS "-fsanitize=address")
add_cache_flag(CMAKE_C_FLAGS "-fsanitize-address-use-after-scope")
add_cache_flag(CMAKE_C_FLAGS "-O1")
add_cache_flag(CMAKE_C_FLAGS "-DNDEBUG")

set(ENV{ASAN_OPTIONS} verbosity=1:debug=1:detect_leaks=1:check_initialization_order=1:alloc_dealloc_mismatch=true:use_odr_indicator=true)
