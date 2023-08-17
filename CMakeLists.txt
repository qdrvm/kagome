cmake_minimum_required(VERSION 3.6)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

set(CMAKE_TOOLCHAIN_FILE
    "${CMAKE_SOURCE_DIR}/cmake/toolchain/cxx17.cmake"
    CACHE FILEPATH "Default toolchain")

include("cmake/Hunter/init.cmake")

add_compile_options(-gdwarf-4)
project(kagome C CXX)

include(cmake/print.cmake)
print("C flags: ${CMAKE_C_FLAGS}")
print("CXX flags: ${CMAKE_CXX_FLAGS}")
print("Using CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)
link_libraries(Threads::Threads)
if(THREADS_HAVE_PTHREAD_ARG)
  link_libraries(pthread)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  link_libraries(atomic)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# cmake-format: off
option(TESTING      "Build and run test suite"                    ON )
option(CLANG_FORMAT "Enable clang-format target"                  ON )
option(CLANG_TIDY   "Enable clang-tidy checks during compilation" OFF)
option(COVERAGE     "Enable generation of coverage info"          OFF)
option(EMBEDDINGS   "Embed developers assets"                     ON )
option(PROFILING    "Enable internal profiling instruments"       OFF)
option(BACKWARD     "Enable stacktrace logging instruments"       ON )
option(CLEAR_OBJS   "Clear object files"                          OFF)

if(NOT($ENV{CI}) OR NOT($ENV{GITHUB_ACTIONS}))
  set(_EXTERNAL_PROJECT_DEFAULT ON)
endif()
option(EXTERNAL_PROJECT "Build external project" ${_EXTERNAL_PROJECT_DEFAULT})

# sanitizers will be enabled only for Kagome, and will be disabled for dependencies
option(ASAN         "Enable address sanitizer"                    OFF)
option(LSAN         "Enable leak sanitizer"                       OFF)
option(MSAN         "Enable memory sanitizer"                     OFF)
option(TSAN         "Enable thread sanitizer"                     OFF)
option(UBSAN        "Enable UB sanitizer"                         OFF)
# cmake-format: on

set(RECOMMENDED_CLANG_FORMAT_VERSION 15)

include(CheckCXXCompilerFlag)
include(cmake/dependencies.cmake)
include(cmake/functions.cmake)

include(cmake/san.cmake)

# setup compilation flags
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(AppleClang|Clang|GNU)$")
  # cmake-format: off

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
  add_flag(-Wtype-limits)            # size_t - size_t >= 0 -> always true

  # disable those flags
  add_flag(-Wno-unused-command-line-argument)    # clang: warning: argument unused during compilation: '--coverage' [-Wunused-command-line-argument]
  add_flag(-Wno-unused-parameter)    # prints too many useless warnings
  add_flag(-Wno-format-nonliteral)   # prints way too many warnings from spdlog
  add_flag(-Wno-gnu-zero-variadic-macro-arguments)   # https://stackoverflow.com/questions/21266380/is-the-gnu-zero-variadic-macro-arguments-safe-to-ignore

  # cmake-format: on
  if((("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(AppleClang|Clang)$")
      AND (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "12.0"))
     OR (("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(GNU)$")
         AND (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "9.0")))
    # use new options format for clang >= 12 and gnu >= 9
    # cmake-format: off
    add_flag(-Werror=unused-lambda-capture)  # error if lambda capture is unused
    add_flag(-Werror=return-type)      # warning: control reaches end of non-void function [-Wreturn-type]
    add_flag(-Werror=sign-compare)     # warn the user if they compare a signed and unsigned numbers
    add_flag(-Werror=reorder)          # field '$1' will be initialized after field '$2'
    add_flag(-Werror=mismatched-tags)  # warning: class '$1' was previously declared as struct
    add_flag(-Werror=switch)  # unhandled values in a switch statement
    # cmake-format: on
  else()
    # promote to errors
    # cmake-format: off
    add_flag(-Werror-unused-lambda-capture)  # error if lambda capture is unused
    add_flag(-Werror-return-type)      # warning: control reaches end of non-void function [-Wreturn-type]
    add_flag(-Werror-non-virtual-dtor) # warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
    add_flag(-Werror-sign-compare)     # warn the user if they compare a signed and unsigned numbers
    add_flag(-Werror-reorder)          # field '$1' will be initialized after field '$2'
    add_flag(-Werror-switch)          # unhandled values in a switch statement
    # cmake-format: on
  endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  # TODO(warchant): add flags
  # https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md#msvc
endif()

print("C flags: ${CMAKE_C_FLAGS}")
print("CXX flags: ${CMAKE_CXX_FLAGS}")

if(COVERAGE)
  include(cmake/coverage.cmake)
endif()
if(CLANG_TIDY)
  include(cmake/clang-tidy.cmake)
endif()
if(CLANG_FORMAT)
  include(cmake/clang-format.cmake)
endif()
if(EMBEDDINGS)
  add_compile_definitions(PRIVATE USE_KAGOME_EMBEDDINGS)
endif()
if(NOT DEFINED PROFILING)
  if(${CMAKE_BUILD_TYPE} EQUALS "Debug")
    set(PROFILING ON)
  endif()
endif()
if(PROFILING)
  add_compile_definitions(PRIVATE KAGOME_PROFILING)
endif()

include(GNUInstallDirs)
include(cmake/install.cmake)

include_directories($<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}/kagome>
                    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/core>)

add_subdirectory(core)

# TODO(Harrm): refactor to avoid manually adding each new directory
kagome_install_setup(
  HEADER_DIRS
  # cmake-format: sortable
  core/application
  core/assets
  core/blockchain
  core/clock
  core/common
  core/consensus
  core/crypto
  core/dispute_coordinator
  core/filesystem
  core/host_api
  core/log
  core/macro
  core/network
  core/offchain
  core/outcome
  core/primitives
  core/runtime
  core/scale
  core/storage
  core/subscription
  core/telemetry
  core/utils)

include(CMakePackageConfigHelpers)

set(CONFIG_INCLUDE_DIRS ${CMAKE_INSTALL_FULL_INCLUDEDIR}/kagome)
configure_package_config_file(
  ${CMAKE_CURRENT_LIST_DIR}/cmake/kagomeConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/kagomeConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kagome)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kagomeConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kagome)

export(PACKAGE kagome)

if(TESTING)
  enable_testing()
  add_subdirectory(test)
endif()

add_subdirectory(node)
