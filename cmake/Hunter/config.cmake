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
    URL  https://github.com/libp2p/cpp-libp2p/archive/32749065f72f169aa1ded0a2d2f0e445c9b10f64.tar.gz
    SHA1 adcd9a1c06b57f2e01c8f127b8e26c9d98bed2e9
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF
    )

hunter_config(soralog
    URL  https://github.com/soramitsu/soralog/archive/e671d31d12525ddf473fb77c083934d67c3fce2a.tar.gz
    SHA1 cddbdf6bc22f35c9d1083b62891d51cf8947da8d
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF
    )

#
#hunter_config(soralog
#    URL  https://github.com/soramitsu/soralog/archive/e671d31d12525ddf473fb77c083934d67c3fce2a.tar.gz
#    SHA1 cddbdf6bc22f35c9d1083b62891d51cf8947da8d
#    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF TSAN=ON
#    KEEP_PACKAGE_SOURCES
#    )
