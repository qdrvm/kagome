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

#hunter_config(
#    leveldb
#    URL  https://github.com/soramitsu/leveldb/archive/c88a9e3757f795630db5cb9a1c2442a4e5ccaed3.tar.gz
#    SHA1 3af764d4596a3ca07f892c1508c917e2692d8ec1
#)
#
#hunter_config(
#    leveldb
#    URL  https://github.com/google/leveldb/archive/refs/tags/1.22.tar.gz
#    SHA1 8d310af5cfb53dc836bfb412ff4b3c8aea578627
#)

hunter_config(
    Сrc32c
    URL "https://github.com/google/crc32c/archive/1.1.1.tar.gz"
    SHA1 "46f18ed84a12c49c42dc77fff54491a9f8b9dce8")

hunter_config(
    libp2p
    URL https://github.com/libp2p/cpp-libp2p/archive/69299a8182a976fbe6654ed367ba1fb5d89800e0.tar.gz
    SHA1 024b4dad4afc900ed15e41fef85ff90e2ea2f00a
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

