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
    URL https://github.com/libp2p/cpp-libp2p/archive/eee88b8ba23daebe337a42e668e5f9af34f48267.tar.gz
    SHA1 a9386e0c0bd1f87428a58d8979cef5a1b3e904b5
    CMAKE_ARGS TESTING=OFF
    )

hunter_config(soralog
    URL  https://github.com/xDimon/soralog/archive/8dee4bba45d3822bf15c7b66031f9c0806c3722f.tar.gz
    SHA1 29bf2edeab4620f1794d3b174accb7bb4a9c4dbe
    )
