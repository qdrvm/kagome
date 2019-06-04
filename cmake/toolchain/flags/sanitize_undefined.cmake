if (DEFINED POLLY_FLAGS_SANITIZE_UNDEFINED_CMAKE_)
  return()
else ()
  set(POLLY_FLAGS_SANITIZE_UNDEFINED_CMAKE_ 1)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/../../add_cache_flag.cmake)

add_cache_flag(CMAKE_CXX_FLAGS "-fsanitize=undefined")
add_cache_flag(CMAKE_CXX_FLAGS "-fno-omit-frame-pointer")
add_cache_flag(CMAKE_CXX_FLAGS "-g")

add_cache_flag(CMAKE_C_FLAGS "-fsanitize=undefined")
add_cache_flag(CMAKE_C_FLAGS "-fno-omit-frame-pointer")
add_cache_flag(CMAKE_C_FLAGS "-g")

set(ENV{UBSAN_OPTIONS} print_stacktrace=1)
