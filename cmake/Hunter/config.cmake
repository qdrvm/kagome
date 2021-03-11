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
    URL https://github.com/libp2p/cpp-libp2p/archive/462cd4a576c1889d302ee847e74a4431d49a14d3.tar.gz
    SHA1 6a5f63aba027f5053a0ce9c492a4c492540f7e7d
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF
    )

hunter_config(soralog
    URL  https://github.com/xDimon/soralog/archive/4a0b1255f86bb641121670abbf2d67268165ee93.tar.gz
    SHA1 827d0ca73ebbe295b14f0906f59ba9d169fc83eb
    )
