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
    URL https://github.com/xDimon/soralog/archive/e9f538034324c022869e6c1fe0cce3014737200b.tar.gz
    SHA1 d1a49c9f0c06d6bb07e5ab865af528351923ee9e
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
)

hunter_config(
    libp2p
    URL https://github.com/libp2p/cpp-libp2p/archive/8d80aa6b984d41ee0d3df0c7b2da01907923dd5b.tar.gz
    SHA1 f5a3cd6f06e0f9cc69ec0bf9a7e5c1ceb628496f
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
)

