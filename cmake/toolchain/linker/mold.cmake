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

include(${CMAKE_CURRENT_LIST_DIR}/../../add_cache_flag.cmake)

set(FLAGS
    -fuse-ld=mold
    )
foreach(FLAG IN LISTS FLAGS)
  add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
  add_cache_flag(CMAKE_C_FLAGS ${FLAG})
endforeach()
