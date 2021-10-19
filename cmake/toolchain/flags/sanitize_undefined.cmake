if (DEFINED POLLY_FLAGS_SANITIZE_UNDEFINED_CMAKE_)
  return()
else ()
  set(POLLY_FLAGS_SANITIZE_UNDEFINED_CMAKE_ 1)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/../../add_cache_flag.cmake)

set(FLAGS
    -fsanitize=undefined
    -fno-omit-frame-pointer
    -g
    )
foreach(FLAG IN LISTS FLAGS)
  add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
  add_cache_flag(CMAKE_C_FLAGS ${FLAG})
endforeach()

add_cache_flag(CMAKE_EXE_LINKER_FLAGS "-fsanitize=undefined")
add_cache_flag(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=undefined")

set(ENV{UBSAN_OPTIONS} print_stacktrace=1)
