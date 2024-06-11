# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

if (DEFINED POLLY_FLAGS_LINKER_MOLD_CMAKE_)
  return()
else ()
  set(POLLY_FLAGS_LINKER_MOLD_CMAKE_ 1)
endif ()

find_program(LLD_FOUND lld)
if(NOT LLD_FOUND)
  fatal_error("lld not found")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../../add_cache_flag.cmake)
add_cache_flag(CMAKE_EXE_LINKER_FLAGS -fuse-ld=lld)
add_cache_flag(CMAKE_SHARED_LINKER_FLAGS -fuse-ld=lld)
