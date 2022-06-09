## Template for a custom hunter configuration
# Useful when there is a need for a non-default version or arguments of a dependency,
# or when a project not registered in soramitsu-hunter should be added.
#
# hunter_config(
#     package-name
#     VERSION 0.0.0-package-version
#     CMAKE_ARGS "CMAKE_VARIABLE=value"
# )
#
# hunter_config(
#     package-name
#     URL https://repo/archive.zip
#     SHA1 1234567890abcdef1234567890abcdef12345678
#     CMAKE_ARGS "CMAKE_VARIABLE=value"
# )


hunter_config(
    backward-cpp
    URL https://github.com/bombela/backward-cpp/archive/refs/tags/v1.6.zip
    SHA1 93c4c843fc9308e62ac462459077d87dc6dd9885
    CMAKE_ARGS BACKWARD_TESTS=OFF
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    soralog
    VERSION 0.0.9
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    libp2p
    VERSION 0.1.3
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    rocksdb
    VERSION 7.2.2
    CMAKE_ARGS WITH_GFLAGS=OFF
)

hunter_config(
    wavm
    VERSION 1.0.3
    CMAKE_ARGS
      TESTING=OFF
      WAVM_ENABLE_FUZZ_TARGETS=OFF
      WAVM_ENABLE_STATIC_LINKING=ON
      WAVM_BUILD_EXAMPLES=OFF
      WAVM_BUILD_TESTS=OFF
    KEEP_PACKAGE_SOURCES
)

