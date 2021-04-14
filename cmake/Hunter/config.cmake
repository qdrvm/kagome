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
    URL  "https://github.com/libp2p/cpp-libp2p/archive/cd477423a44e0c4f0690881bf79a8d97057af907.tar.gz"
    SHA1 "fe21851f47c9260deb1fd445fff19f7d25e47226"
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF
    KEEP_PACKAGE_SOURCES
    )

