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
    qtils
    VERSION 0.1.0
    CMAKE_ARGS
      FORMAT_ERROR_WITH_FULLTYPE=ON
)

hunter_config(
    backward-cpp
    URL https://github.com/bombela/backward-cpp/archive/refs/tags/v1.6.zip
    SHA1 93c4c843fc9308e62ac462459077d87dc6dd9885
    CMAKE_ARGS BACKWARD_TESTS=OFF
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    rocksdb
    VERSION 9.6.1
    CMAKE_ARGS WITH_GFLAGS=OFF USE_RTTI=ON
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

  set(ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")

  if(ARCHITECTURE MATCHES "^(aarch64.*|AARCH64.*|arm.*|ARM.*)")
      set(ARCHITECTURE AArch64)
  elseif(ARCHITECTURE MATCHES "^(x86_64.*|AMD64.*|i386.*|i686.*)")
      set(ARCHITECTURE X86)
  elseif(ARCHITECTURE MATCHES "^(riscv.*)")
      set(ARCHITECTURE RISCV)
  else()
      message(WARNING "Unknown architecture: ${ARCHITECTURE}, using all architectures to build LLVM")
      set(ARCHITECTURE AArch64;X86;RISCV)
  endif()

  hunter_config(
    LLVM
    VERSION 17.0.6
    CMAKE_ARGS # inspired by https://github.com/WasmEdge/WasmEdge/blob/5e8556afa5a71f3d3ef9615334ecf1a9d4d0f1e8/utils/docker/Dockerfile.manylinux2014_x86_64#L57
        LLVM_ENABLE_PROJECTS=lld;clang
        LLVM_TARGETS_TO_BUILD=${ARCHITECTURE};BPF 
  )

  hunter_config(
      WasmEdge
      URL https://github.com/qdrvm/WasmEdge/archive/refs/tags/0.14.1.zip
      SHA1 ${WASMEDGE_ID}
      CMAKE_ARGS
        WASMEDGE_BUILD_STATIC_LIB=ON
        WASMEDGE_BUILD_SHARED_LIB=OFF
        CMAKE_CXX_FLAGS=-Wno-error=maybe-uninitialized
      KEEP_PACKAGE_SOURCES
  )
endif ()

if ("${WASM_COMPILER}" STREQUAL "WAVM")
  hunter_config(
      LLVM
      VERSION 12.0.1-p4
      CMAKE_ARGS
        LLVM_ENABLE_PROJECTS=ir
        LLVM_TARGETS_TO_BUILD="AArch64;ARM;X86"
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
    URL  https://github.com/qdrvm/kagome-crates/archive/refs/tags/v1.0.4.tar.gz
    SHA1 a85f3ca7a5dac2d22c609cc0c3f39408ad72dba8
)

hunter_config(
    libsecp256k1
    VERSION 0.5.1
    CMAKE_ARGS
      SECP256K1_ENABLE_MODULE_RECOVERY=ON
)

hunter_config(
    wabt
    URL https://github.com/qdrvm/wabt/archive/2e9d30c4a67c1b884a8162bf3f3a5a8585cfdb94.tar.gz
    SHA1 b5759660eb8ad3f074274341641e918f688868bd
    KEEP_PACKAGE_SOURCES
)
