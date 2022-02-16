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
    URL https://github.com/igor-egorov/cpp-libp2p/archive/feature/ecdsa-prehashed-face.tar.gz
    SHA1 e45549cf6b8e22b6769766044354fc2493050493
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    wavm
    URL "https://github.com/soramitsu/WAVM/archive/refs/tags/1.0.3.zip"
    SHA1 67cafaec3c810a5e8d7bb9416148d7532a0071ed
    CMAKE_ARGS
      TESTING=OFF
      WAVM_ENABLE_FUZZ_TARGETS=OFF
      WAVM_ENABLE_STATIC_LINKING=ON
      WAVM_BUILD_EXAMPLES=OFF
      WAVM_BUILD_TESTS=OFF
    KEEP_PACKAGE_SOURCES
)

