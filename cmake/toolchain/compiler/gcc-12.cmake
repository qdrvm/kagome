if(DEFINED POLLY_COMPILER_GCC_12_CMAKE_)
  return()
else()
  set(POLLY_COMPILER_GCC_12_CMAKE_ 1)
endif()

find_program(CMAKE_C_COMPILER gcc-12)
find_program(CMAKE_CXX_COMPILER g++-12)

if(NOT CMAKE_C_COMPILER)
  fatal_error("gcc-12 not found")
endif()

if(NOT CMAKE_CXX_COMPILER)
  fatal_error("g++-12 not found")
endif()

set(
    CMAKE_C_COMPILER
    "${CMAKE_C_COMPILER}"
    CACHE
    STRING
    "C compiler"
    FORCE
)

set(
    CMAKE_CXX_COMPILER
    "${CMAKE_CXX_COMPILER}"
    CACHE
    STRING
    "C++ compiler"
    FORCE
)
