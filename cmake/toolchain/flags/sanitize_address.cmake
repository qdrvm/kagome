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
    -DNDEBUG
    )
foreach(FLAG IN LISTS FLAGS)
  add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
  add_cache_flag(CMAKE_C_FLAGS ${FLAG})
endforeach()

add_cache_flag(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")
add_cache_flag(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=address")

add_compile_definitions(KAGOME_WITH_ASAN)

message(STATUS "Address sanitizer is active")
