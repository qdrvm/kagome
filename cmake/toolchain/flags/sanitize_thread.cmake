# Copyright (c) 2014-2017, Ruslan Baratov
# All rights reserved.

if (DEFINED POLLY_FLAGS_SANITIZE_THREAD_CMAKE_)
  return()
else ()
  set(POLLY_FLAGS_SANITIZE_THREAD_CMAKE_ 1)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/../../add_cache_flag.cmake)

set(TSAN_IGNORELIST "${CMAKE_CURRENT_LIST_DIR}/../../../.thread-sanitizer-ignore")

list(APPEND KAGOME_CTEST_ENV "TSAN_OPTIONS=suppressions=${TSAN_IGNORELIST}")

set(FLAGS
    -fsanitize=thread
    -g
    -O1
)
if (CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set(FLAGS
      ${FLAGS}
      -fsanitize-blacklist="${TSAN_IGNORELIST}"
      -fsanitize-ignorelist="${TSAN_IGNORELIST}"
  )
endif()

foreach(FLAG IN LISTS FLAGS)
  add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
  add_cache_flag(CMAKE_C_FLAGS ${FLAG})
endforeach()

add_cache_flag(CMAKE_EXE_LINKER_FLAGS "-fsanitize=thread")
add_cache_flag(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=thread")

message(STATUS "Thread sanitizer has activated")
