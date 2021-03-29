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
    URL  "https://github.com/libp2p/cpp-libp2p/archive/bddea32b9329d4a2b07a3747bc4953f13caa0747.tar.gz"
    SHA1 "ffc5f02d6e657042830ef64a125442999e2cf05d"
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF
    )
