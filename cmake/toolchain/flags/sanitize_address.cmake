# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

if (DEFINED POLLY_FLAGS_SANITIZE_ADDRESS_CMAKE_)
  return()
else ()
  set(POLLY_FLAGS_SANITIZE_ADDRESS_CMAKE_ 1)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/../../add_cache_flag.cmake)

set(FLAGS
    -fsanitize=address
    -fsanitize-address-use-after-scope
    -fno-omit-frame-pointer
    -g
    -O1
    -DNDEBUG
    )
foreach(FLAG IN LISTS FLAGS)
  add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
  add_cache_flag(CMAKE_C_FLAGS ${FLAG})
endforeach()

add_cache_flag(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")
add_cache_flag(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=address")

set(ENV{ASAN_OPTIONS} verbosity=1:debug=1:detect_leaks=1:check_initialization_order=1:alloc_dealloc_mismatch=true:use_odr_indicator=true)
