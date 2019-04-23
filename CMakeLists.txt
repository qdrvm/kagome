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

include(CheckCXXCompilerFlag)
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


## setup compilation flags
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(AppleClang|Clang|GNU)$")
  # enable those flags
  add_flag(-Wall)
  add_flag(-Wextra)
  add_flag(-Woverloaded-virtual)     # warn if you overload (not override) a virtual function
  add_flag(-Wformat=2)               # warn on security issues around functions that format output (ie printf)
  add_flag(-Wmisleading-indentation) # (only in GCC >= 6.0) warn if indentation implies blocks where blocks do not exist
  add_flag(-Wduplicated-cond)        # (only in GCC >= 6.0) warn if if / else chain has duplicated conditions
  add_flag(-Wduplicated-branches)    # (only in GCC >= 7.0) warn if if / else branches have duplicated code
  add_flag(-Wnull-dereference)       # (only in GCC >= 6.0) warn if a null dereference is detected
  add_flag(-Wdouble-promotion)       # (GCC >= 4.6, Clang >= 3.8) warn if float is implicit promoted to double
  add_flag(-Wsign-compare)

  # disable those flags
  add_flag(-Wno-unused-parameter)    # prints too many useless warnings
  add_flag(-Wno-format-nonliteral)   # prints way too many warnings from spdlog
  add_flag(-Wno-gnu-zero-variadic-macro-arguments)   # https://stackoverflow.com/questions/21266380/is-the-gnu-zero-variadic-macro-arguments-safe-to-ignore

  # promote to errors
  add_flag(-Werror-unused-lambda-capture)  # error if lambda capture is unused
  add_flag(-Werror-return-type)      # warning: control reaches end of non-void function [-Wreturn-type]
  add_flag(-Werror-non-virtual-dtor) # warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors

  # TODO(warchant): FIX WARNING: comparison of signed and unsigned; then uncomment
#  add_flag(-Werror-sign-compare)     # warn the user if they compare a signed and unsigned numbers
  add_flag(-Werror-reorder)          # field '$1' will be initialized after field '$2'
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  # TODO(warchant): add flags https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md#msvc
endif()

if(COVERAGE)
  include(cmake/coverage.cmake)
endif()
if(CLANG_TIDY)
  include(cmake/clang-tidy.cmake)
endif()
if(CLANG_FORMAT)
  include(cmake/clang-format.cmake)
endif()

add_subdirectory(deps)

include_directories(
  # project includes
  ${PROJECT_SOURCE_DIR}/core
)

include_directories(
  SYSTEM
  # system includes
  deps/spdlog/include
  deps/GSL/include
  deps/outcome
  deps/sml/include
)

add_subdirectory(core)

enable_testing()
add_subdirectory(test)
