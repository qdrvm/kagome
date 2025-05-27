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
    VERSION 0.1.1
    CMAKE_ARGS
      FORMAT_ERROR_WITH_FULLTYPE=ON
)

hunter_config(
    backward-cpp
    URL https://github.com/bombela/backward-cpp/archive/refs/tags/v1.6.zip
    SHA1 93c4c843fc9308e62ac462459077d87dc6dd9885
    CMAKE_ARGS
      BACKWARD_TESTS=OFF
      CMAKE_POLICY_VERSION_MINIMUM=3.5      
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    rocksdb
    VERSION 9.6.1
    CMAKE_ARGS
      WITH_GFLAGS=OFF
      USE_RTTI=ON
)

if ("${WASM_COMPILER}" STREQUAL "WasmEdge")
  hunter_config(
      fmt
      URL  https://github.com/fmtlib/fmt/archive/refs/tags/10.2.1.tar.gz
      SHA1 d223964b782d2562d6722ffe67027204c6035453
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
        CMAKE_POLICY_VERSION_MINIMUM=3.5        
      KEEP_PACKAGE_SOURCES
  )
endif ()

hunter_config(
    kagome-crates
    URL  https://github.com/qdrvm/kagome-crates/archive/refs/tags/v1.0.5.tar.gz
    SHA1 81248ac44aef8c6249f11de1b0e9b5be6b7c810d
)

hunter_config(
    libsecp256k1
    VERSION 0.5.1
    CMAKE_ARGS
      SECP256K1_ENABLE_MODULE_RECOVERY=ON
)

hunter_config(
    libp2p
    URL https://github.com/libp2p/cpp-libp2p/archive/96689b3828b7a18ac197728a69a744dbc27ac2e6.tar.gz
    SHA1 6e9c0ef45a44fbf710d087efee921e1aa7de6638
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    wabt
    URL https://github.com/qdrvm/wabt/archive/2e9d30c4a67c1b884a8162bf3f3a5a8585cfdb94.tar.gz
    SHA1 b5759660eb8ad3f074274341641e918f688868bd
    KEEP_PACKAGE_SOURCES
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(BORINGSSL_C_FLAGS -Wno-stringop-overflow)
  set(BORINGSSL_CXX_FLAGS -Wno-stringop-overflow)
else()
  set(BORINGSSL_C_FLAGS -Wno-error)
  set(BORINGSSL_CXX_FLAGS -Wno-error)
endif()

hunter_config(
    BoringSSL
    VERSION qdrvm1
    CMAKE_ARGS
      CMAKE_C_FLAGS=${BORINGSSL_C_FLAGS}
      CMAKE_CXX_FLAGS=${BORINGSSL_CXX_FLAGS}
      CMAKE_THREAD_LIBS_INIT=-lpthread
      CMAKE_HAVE_THREADS_LIBRARY=1
      CMAKE_USE_WIN32_THREADS_INIT=0
      CMAKE_USE_PTHREADS_INIT=1
      THREADS_PREFER_PTHREAD_FLAG=ON
)

hunter_config(
    binaryen
    VERSION 1.38.28-patch.3
    CMAKE_ARGS
      CMAKE_POLICY_VERSION_MINIMUM=3.5
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    Protobuf
    VERSION 3.19.4-p0
    CMAKE_ARGS
      CMAKE_POLICY_VERSION_MINIMUM=3.5
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    c-ares
    VERSION 1.14.0-p0
    CMAKE_ARGS
      CMAKE_POLICY_VERSION_MINIMUM=3.5
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    yaml-cpp
    VERSION 0.6.2-p0
    CMAKE_ARGS
      CMAKE_POLICY_VERSION_MINIMUM=3.5
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    RapidJSON
    VERSION 1.1.0-66eb606-p0
    CMAKE_ARGS
      CMAKE_POLICY_VERSION_MINIMUM=3.5
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    jsonrpc-lean
    VERSION 0.0.0-6c093da8
    CMAKE_ARGS
      CMAKE_POLICY_VERSION_MINIMUM=3.5
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    Boost.DI
    VERSION 1.1.0
    CMAKE_ARGS
      CMAKE_POLICY_VERSION_MINIMUM=3.5
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    zstd
    VERSION 1.4.5-d73e2fb-p0
    CMAKE_ARGS
      CMAKE_POLICY_VERSION_MINIMUM=3.5
    KEEP_PACKAGE_SOURCES
)
