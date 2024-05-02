if(DEFINED POLLY_COMPILER_CLANG_15_CMAKE)
  return()
else()
  set(POLLY_COMPILER_CLANG_15_CMAKE 1)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../../print.cmake)

if(XCODE_VERSION)
  set(_err "This toolchain is not available for Xcode")
  set(_err "${_err} because Xcode ignores CMAKE_C(XX)_COMPILER variable.")
  set(_err "${_err} Use xcode.cmake toolchain instead.")
  fatal_error(${_err})
endif()

find_program(CMAKE_C_COMPILER clang-15 REQUIRED)
find_program(CMAKE_CXX_COMPILER clang++-15)

if (CMAKE_CXX_COMPILER STREQUAL "CMAKE_CXX_COMPILER-NOTFOUND")
  message(STATUS "clang++-15 not found, checking clang++")
  cmake_path(GET CMAKE_C_COMPILER PARENT_PATH compiler_path)
  message(STATUS "Assumed compiler path: ${compiler_path}")
  # clang++-15 doesn't always exist
  find_program(CMAKE_CXX_COMPILER clang++ PATHS "${compiler_path}" NO_DEFAULT_PATH REQUIRED)

  execute_process(COMMAND "${CMAKE_CXX_COMPILER}" --version OUTPUT_VARIABLE compiler_version_output)
  string(REGEX MATCH "clang version ([0-9]+)\\.[0-9]+\\.[0-9]+" compiler_version "${compiler_version_output}")
  if (NOT CMAKE_MATCH_1 STREQUAL "15")
    message(FATAL_ERROR "Found clang++ version ${CMAKE_MATCH_1}, 15 is required")
  endif()
endif()

if(NOT CMAKE_C_COMPILER)
  fatal_error("clang-15 not found")
endif()

if(NOT CMAKE_CXX_COMPILER)
  fatal_error("clang++-15 not found")
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
