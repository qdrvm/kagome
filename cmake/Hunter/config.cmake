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
    URL  "https://github.com/libp2p/cpp-libp2p/archive/2ea76cd4b4f4fb236f4a45351860d0ec0e093358.tar.gz"
    SHA1 "05cffbaa2701ca413dd4220c98b5e19e557a6695"
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
    )

#hunter_config(libp2p
#    URL  "https://github.com/libp2p/cpp-libp2p/archive/c9fb62437133ddcd5d17c548aa9f0fa5eab3f59d.tar.gz"
#    SHA1 "e7a082e9c98796e15f7293aee0640210d265ce6a"
#    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
#    KEEP_PACKAGE_SOURCES
#    )
