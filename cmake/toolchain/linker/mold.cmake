# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

if (DEFINED POLLY_FLAGS_LINKER_MOLD_CMAKE_)
  return()
else ()
  set(POLLY_FLAGS_LINKER_MOLD_CMAKE_ 1)
endif ()

find_program(MOLD_FOUND mold)
if(NOT MOLD_FOUND)
  fatal_error("mold not found")
endif()

include(add_cache_flag.cmake)
add_cache_flag(CMAKE_EXE_LINKER_FLAGS -fuse-ld=mold)
