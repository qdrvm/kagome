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
    URL  https://github.com/libp2p/cpp-libp2p/archive/31121af6bd6094c72d57b63b66c6e0040f731c9a.tar.gz
    SHA1 c88fa98265932e308c48b63de0d4ace9c0e64ae5
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF
    )
