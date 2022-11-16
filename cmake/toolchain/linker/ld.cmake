# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

if (DEFINED POLLY_FLAGS_LINKER_LD_CMAKE_)
  return()
else ()
  set(POLLY_FLAGS_LINKER_LD_CMAKE_ 1)
endif ()

find_program(LD_FOUND ld)
if(NOT LD_FOUND)
  fatal_error("ld not found")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../../add_cache_flag.cmake)
add_cache_flag(CMAKE_EXE_LINKER_FLAGS -Wl,--reduce-memory-overheads)
add_cache_flag(CMAKE_EXE_LINKER_FLAGS -Wl,--hash-size=1021)


