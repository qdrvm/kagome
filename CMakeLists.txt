cmake_minimum_required(VERSION 3.12)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)


set(
    CMAKE_TOOLCHAIN_FILE
    "${CMAKE_SOURCE_DIR}/cmake/toolchain/cxx17.cmake"
    CACHE
    FILEPATH
    "Default toolchain"
)

include("cmake/Hunter/init.cmake")
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.23.171.tar.gz"
    SHA1 "5d68bcca78eee347239ca5f4d34f4b6c12683154"
    FILEPATH ${CMAKE_SOURCE_DIR}/cmake/Hunter/config.cmake
)

PROJECT(kagome C CXX)

include(cmake/print.cmake)
print("C flags: ${CMAKE_C_FLAGS}")
print("CXX flags: ${CMAKE_CXX_FLAGS}")
print("Using CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)
link_libraries(Threads::Threads)
if(THREADS_HAVE_PTHREAD_ARG)
  add_link_options(-lpthread)
endif()


include(CheckCXXCompilerFlag)
include(cmake/dependencies.cmake)
include(cmake/functions.cmake)
include(cmake/san.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CLANG_FORMAT "Enable clang-format target"                  ON )
option(CLANG_TIDY   "Enable clang-tidy checks during compilation" OFF)
option(COVERAGE     "Enable generation of coverage info"          OFF)
# sanitizers will be enabled only for Kagome, and will be disabled for dependencies
option(ASAN         "Enable address sanitizer"                    OFF)
option(LSAN         "Enable leak sanitizer"                       OFF)
option(MSAN         "Enable memory sanitizer"                     OFF)
option(TSAN         "Enable thread sanitizer"                     OFF)
option(UBSAN        "Enable UB sanitizer"                         OFF)


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
  add_flag(-Wtype-limits)            # size_t - size_t >= 0 -> always true

  # disable those flags
  add_flag(-Wno-unused-command-line-argument)    # clang: warning: argument unused during compilation: '--coverage' [-Wunused-command-line-argument]
  add_flag(-Wno-unused-parameter)    # prints too many useless warnings
  add_flag(-Wno-format-nonliteral)   # prints way too many warnings from spdlog
  add_flag(-Wno-gnu-zero-variadic-macro-arguments)   # https://stackoverflow.com/questions/21266380/is-the-gnu-zero-variadic-macro-arguments-safe-to-ignore

  # promote to errors
  add_flag(-Werror-unused-lambda-capture)  # error if lambda capture is unused
  add_flag(-Werror-return-type)      # warning: control reaches end of non-void function [-Wreturn-type]
  add_flag(-Werror-non-virtual-dtor) # warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
  add_flag(-Werror-sign-compare)     # warn the user if they compare a signed and unsigned numbers
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
  deps/sr25519-crust/include
  deps/hat-trie/include
  deps/jsonrpc-lean/include
  deps/di/include
  deps/di/extension/include
)

add_subdirectory(core)

include(GNUInstallDirs)

include(cmake/install.cmake)

install_deps_headers()

kagome_install(
    TARGETS
    scale
    buffer
    hexutil
    blob
    polkadot_trie_db
    polkadot_trie_codec
    polkadot_trie
    polkadot_node
    blake2
    hasher
    twox
    sha
    polkadot_trie_batch
    leveldb
    logger
    in_memory_storage

    HEADER_DIRS
    core/common
    core/macro
    core/scale
    core/storage
    core/crypto
    )

include(CMakePackageConfigHelpers)

set(CONFIG_INCLUDE_DIRS ${CMAKE_INSTALL_FULL_INCLUDEDIR}/kagome)
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/kagomeConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/kagomeConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kagome
    )

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kagomeConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kagome
    )

export(PACKAGE kagome)

enable_testing()
add_subdirectory(test)
