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
    binaryen
    URL https://github.com/qdrvm/binaryen/archive/0744f64a584cae5b9255b1c2f0a4e0b5e06d7038.zip
    SHA1 f953c5f38a0417e494901e15ab6f5d8267388d18
)

hunter_config(
    soralog
    VERSION 0.2.1
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    libp2p
    VERSION 0.1.15
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    rocksdb
    VERSION 8.0.0
    CMAKE_ARGS WITH_GFLAGS=OFF
)

hunter_config(
    LLVM
    URL  https://github.com/qdrvm/kagome-llvm/archive/refs/tags/v13.0.1.tar.gz
    SHA1 a36890190449798e6bbec1e6061544d7016859d8
    CONFIGURATION_TYPES
      Release
    CMAKE_ARGS
      LLVM_ENABLE_PROJECTS=clang;clang-tools-extra;compiler-rt
      LLVM_ENABLE_ZLIB=OFF
      LLVM_INCLUDE_EXAMPLES=OFF
      LLVM_INCLUDE_TESTS=OFF
      LLVM_INCLUDE_DOCS=OFF
      LLVM_PARALLEL_LINK_JOBS=1
)

hunter_config(
    wavm
    URL  https://github.com/qdrvm/WAVM/archive/5afb4e81f4976ee36b9847acaaf46c967ef479fe.tar.gz
    SHA1 f5abc08c97e10e6683ef3d1d3bf64b0dccf7a5e6
    CMAKE_ARGS
      WAVM_ENABLE_FUZZ_TARGETS=OFF
      WAVM_ENABLE_STATIC_LINKING=ON
      WAVM_BUILD_EXAMPLES=OFF
      WAVM_BUILD_TESTS=OFF
      WAVM_DISABLE_UNIX_SIGNALS=ON
)
