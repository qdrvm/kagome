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
    URL  https://github.com/qdrvm/kagome-llvm/archive/refs/tags/v12.0.1-p4.tar.gz
    SHA1 3c4c5700710fc0e33d3548fce6041ff85f06b960
    CMAKE_ARGS
    LLVM_ENABLE_PROJECTS=ir
    KEEP_PACKAGE_SOURCES
)

# Fix for Apple clang (or clang from brew) of versions 15 and higher
if(APPLE AND (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL "15.0.0")
    hunter_config(
        binaryen
        URL https://github.com/qdrvm/binaryen/archive/0744f64a584cae5b9255b1c2f0a4e0b5e06d7038.zip
        SHA1 f953c5f38a0417e494901e15ab6f5d8267388d18
    )
endif()
