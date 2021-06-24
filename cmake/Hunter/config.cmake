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
    URL  https://github.com/libp2p/cpp-libp2p/archive/393119ee18a6a3f6b8d8deeaaa77e2ad401bd8cb.tar.gz
    SHA1 31886c5235f60acba48bd2455b53e34f48c9b7fc
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
    )

hunter_config(prometheus-cpp
    URL "https://github.com/jupp0r/prometheus-cpp/releases/download/v0.12.3/prometheus-cpp-with-submodules.tar.gz"
    SHA1 0e337607d4ce55cdc80c37b60dcfe4dbbf7812cf
    CMAKE_ARGS ENABLE_TESTING=OFF USE_THIRDPARTY_LIBRARIES=OFF OVERRIDE_CXX_STANDARD_FLAGS=OFF ENABLE_PULL=OFF ENABLE_PUSH=OFF ENABLE_COMPRESSION=OFF
    )
