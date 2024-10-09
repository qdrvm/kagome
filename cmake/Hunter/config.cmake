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
    URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
    SHA1 bf9870756ee3f8d2d3b346b24ee3600a41c74d3d
    CMAKE_ARGS BENCHMARK_ENABLE_TESTING=OFF
)

hunter_config(
    rocksdb
    VERSION 9.0.0
    CMAKE_ARGS WITH_GFLAGS=OFF
)

if ("${WASM_COMPILER}" STREQUAL "WasmEdge")
  hunter_config(
      fmt
      URL
          https://github.com/fmtlib/fmt/archive/refs/tags/10.2.1.tar.gz
      SHA1
          d223964b782d2562d6722ffe67027204c6035453
      CMAKE_ARGS
          CMAKE_POSITION_INDEPENDENT_CODE=TRUE
  )

  hunter_config(
      spdlog
      VERSION 1.12.0-p0
      CMAKE_ARGS
          SPDLOG_BUILD_PIC=ON
          SPDLOG_FMT_EXTERNAL=ON
  )

  hunter_config(
      WasmEdge
      URL  https://github.com/qdrvm/WasmEdge/archive/refs/heads/update/0.14.0.zip
      SHA1 ${WASMEDGE_ID}
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
    kagome-crates
    URL  https://github.com/qdrvm/kagome-crates/archive/refs/tags/1.0.2.tar.gz
    SHA1 946c48508545380e155ab831be54228b916544d3
)

hunter_config(
    libsecp256k1
    VERSION 0.5.1
    CMAKE_ARGS
      SECP256K1_ENABLE_MODULE_RECOVERY=ON
)

hunter_config(
    libp2p
    URL https://github.com/libp2p/cpp-libp2p/archive/a27afe888ff0fcab03c4f3f0e7bd33973d03fb09.tar.gz
    SHA1 2387bd00388be95b9771d8b1811087f85037c6ea
)

hunter_config(lsquic
    URL  https://github.com/qdrvm/lsquic/archive/958701ee1286f5ece3cd1354ee8df7d39ef24faf.zip
    SHA1 397dae10ff52d56dadd2f11f6d8de759b6ab768f
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    erasure_coding_crust
#    VERSION 0.0.8
    URL  https://github.com/qdrvm/erasure-coding-crust/archive/refs/tags/v0.0.8.tar.gz
    SHA1 6bcdb6327f5da2dcec5c70f2fa63b95a44925af0
    KEEP_PACKAGE_SOURCES
)

