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
    URL  https://github.com/libp2p/cpp-libp2p/archive/8732a78525c5cf086388b39cf9986d9a93e56ebc.tar.gz
    SHA1 5266290fdc893d5a65dd8caa8ed60f6c7f1ae32a
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF
    )
