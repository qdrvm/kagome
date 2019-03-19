cmake_minimum_required(VERSION 3.12)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

include("cmake/Hunter/init.cmake")
HunterGate(
  URL "https://github.com/ruslo/hunter/archive/v0.23.135.tar.gz"
  SHA1 "72eecdf1dea9fb6299d7fb61bbd00bd4ca170fa4"
)

PROJECT(kagome C CXX)

include(cmake/dependencies.cmake)
include(cmake/functions.cmake)

if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
  # https://cgold.readthedocs.io/en/latest/tutorials/toolchain/globals/cxx-standard.html#summary
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CLANG_FORMAT "Enable clang-format target"                  ON )
option(CLANG_TIDY   "Enable clang-tidy checks during compilation" OFF)
option(COVERAGE     "Enable generation of coverage info"          OFF)

if(COVERAGE)
  include(cmake/coverage.cmake)
endif()
if(CLANG_TIDY)
  include(cmake/clang-tidy.cmake)
endif()
if(CLANG_FORMAT)
  include(cmake/clang-format.cmake)
endif()

include_directories(
  ${PROJECT_SOURCE_DIR}/core
  # add path to header only deps here
  deps/spdlog/include
  deps/RxCpp/Rx/v2/src
  deps/GSL/include
)

add_subdirectory(core)

enable_testing()
add_subdirectory(test)
