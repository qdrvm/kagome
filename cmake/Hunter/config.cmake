## Template for a custom hunter configuration
# Useful when there is a need for a non-default version or arguments of a dependency,
# or when a project not registered in soramistu-hunter should be added.
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
    soralog
    URL https://github.com/soramitsu/soralog/archive/v0.0.9.tar.gz
    SHA1 a5df392c969136e9cb2891d7cc14e3e6d34107d6
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    libp2p
    URL https://github.com/libp2p/cpp-libp2p/archive/96328c72f7d7aeb614c515cffe94b8282da20f22.tar.gz
    SHA1 1b2af24d2590d5d8b61c0f3ecd33900a9cd362ab
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    wavm
    URL https://github.com/soramitsu/WAVM/archive/9b914111910e9ece1a9243923059d16c153f4c9f.zip
    SHA1 519484aec89f6e2debdddd1ce0ab68d900c19ff4
    CMAKE_ARGS
    WAVM_BUILD_EXAMPLES=OFF
    WAVM_BUILD_TESTS=OFF
    WAVM_ENABLE_FUZZ_TARGETS=OFF
    WAVM_ENABLE_STATIC_LINKING=ON
    KEEP_PACKAGE_SOURCES
)
