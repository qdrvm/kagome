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

hunter_config(libp2p
    URL https://github.com/libp2p/cpp-libp2p/archive/5377b9d19483ef05d81ef5499cf871e9d965aca0.tar.gz
    SHA1 af38acc44b624b76b295590c0d1adcc9af9b5c04
    CMAKE_ARGS TESTING=OFF
    )
