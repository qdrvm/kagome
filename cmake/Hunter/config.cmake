# Template for a custom hunter configuration Useful when there is a need for a
# non-default version or arguments of a dependency, or when a project not
# registered in soramitsu-hunter should be added.
#
# hunter_config(
#     package-name
#     VERSION 0.0.0-package-version
#     CMAKE_ARGS
#      CMAKE_VARIABLE=value
# )
#
# hunter_config(
#     package-name
#     URL https://repo/archive.zip
#     SHA1 1234567890abcdef1234567890abcdef12345678
#     CMAKE_ARGS
#       CMAKE_VARIABLE=value
# )

hunter_config(
    backward-cpp
    URL https://github.com/bombela/backward-cpp/archive/refs/tags/v1.6.zip
    SHA1 93c4c843fc9308e62ac462459077d87dc6dd9885
    CMAKE_ARGS BACKWARD_TESTS=OFF
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    benchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.7.1.zip
    SHA1 988246a257b0eeb1a8b112cff6ab3edfbe162912
    CMAKE_ARGS BENCHMARK_ENABLE_TESTING=OFF
)

hunter_config(
    soralog
    VERSION 0.2.1
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    libp2p
    VERSION 0.1.18
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    rocksdb
    VERSION 8.0.0
    CMAKE_ARGS WITH_GFLAGS=OFF
)

if ("${WASM_COMPILER}" STREQUAL "WasmEdge")
  hunter_config(
      WasmEdge
      VERSION 0.14.0-alpha.1-qdrvm3
      CMAKE_ARGS
        WASMEDGE_BUILD_STATIC_LIB=ON
        WASMEDGE_BUILD_SHARED_LIB=OFF
      KEEP_PACKAGE_SOURCES
  )
endif ()

if ("${WASM_COMPILER}" STREQUAL "WAVM")
  hunter_config(
      LLVM
      VERSION 12.0.1-p4
      CMAKE_ARGS
      LLVM_ENABLE_PROJECTS=ir
      KEEP_PACKAGE_SOURCES
  )

  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(WAVM_CXX_FLAGS -Wno-redundant-move;-Wno-dangling-reference;-Wno-error=extra;)
  else ()
    set(WAVM_CXX_FLAGS -Wno-redundant-move)
  endif ()

  hunter_config(
      wavm
      VERSION 1.0.14
      CMAKE_ARGS
      WAVM_CXX_FLAGS=${WAVM_CXX_FLAGS}
      KEEP_PACKAGE_SOURCES
  )
endif ()

hunter_config(
    scale
    VERSION 1.1.2
    KEEP_PACKAGE_SOURCES
)

# Fix for Apple clang (or clang from brew) of versions 15 and higher
if (APPLE AND (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL "15.0.0")
  hunter_config(
      binaryen
      URL https://github.com/qdrvm/binaryen/archive/e6a2fea157bde503f07f28444b350512374cf5bf.zip
      SHA1 301f8b1775904179cb552c12be237b4aa076981e
  )
endif ()
