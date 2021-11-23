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
    URL https://github.com/libp2p/cpp-libp2p/archive/69299a8182a976fbe6654ed367ba1fb5d89800e0.tar.gz
    SHA1 024b4dad4afc900ed15e41fef85ff90e2ea2f00a
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    schnorrkel_crust
    URL https://github.com/soramitsu/soramitsu-sr25519-crust/archive/105d97b03179bcfb308f90c732472817aa61ed24.tar.gz
    SHA1 d03f82a53ba39c3b4eb9380afb3dbd4f810683bb
    CMAKE_ARGS
        TESTING=OFF
    KEEP_PACKAGE_SOURCES
)

hunter_config(
  wavm
  URL https://github.com/soramitsu/WAVM/archive/c3f1a9e5ff7db38c722bdbfd79cf9edc325ff1c9.tar.gz
  SHA1 637a9c27efdfaef20f2173b2b75782506242f07c
  CMAKE_ARGS
      WAVM_ENABLE_FUZZ_TARGETS=OFF
      WAVM_ENABLE_STATIC_LINKING=ON
      WAVM_BUILD_EXAMPLES=OFF
      WAVM_BUILD_TESTS=OFF
  KEEP_PACKAGE_SOURCES
)

