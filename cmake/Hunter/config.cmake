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

message(FATAL_ERROR "Kagome config")

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
    URL https://github.com/libp2p/cpp-libp2p/archive/refs/heads/feature/cmake-config.tar.gz
    SHA1 edae6a868637e6288eca44f17747bd3cd4c784a3
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    schnorrkel_crust
    URL https://github.com/soramitsu/soramitsu-sr25519-crust/archive/refs/heads/refactor/cmake-config.tar.gz
    SHA1 c35cc25fda7a57003c2255241d9b1b2cc252f701
    KEEP_PACKAGE_SOURCES
)

hunter_config(
  wavm
  URL https://github.com/soramitsu/WAVM/archive/7efbcced0d41d5f7bc6cd254d624e5f7174b54fc.tar.gz
  SHA1 dd22c11c5faf95a6f6b05ecff18f0e8cab475771
  CMAKE_ARGS
      WAVM_ENABLE_FUZZ_TARGETS=OFF
      WAVM_ENABLE_STATIC_LINKING=ON
      WAVM_BUILD_EXAMPLES=OFF
      WAVM_BUILD_TESTS=OFF
  KEEP_PACKAGE_SOURCES
)
