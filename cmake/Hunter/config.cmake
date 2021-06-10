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
    URL  "https://github.com/libp2p/cpp-libp2p/archive/d51a7f7d6252d54f926fc0226affe8193505103f.tar.gz"
    SHA1 "12ffce5ebdf2f1429f3ea218da59b50bce530ec0"
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
    )

hunter_config(soralog
    URL  "https://github.com/xDimon/soralog/archive/8fbddeef627eeb0456910ba92e89a5b4d9cd75ed.tar.gz"
    SHA1 "98fa62ef43753ad80b2d64756aaa6fa6a49625c1"
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF EXPOSE_MOCKS=ON
    KEEP_PACKAGE_SOURCES
    )

hunter_config(prometheus-cpp
    URL "https://github.com/jupp0r/prometheus-cpp/releases/download/v0.12.3/prometheus-cpp-with-submodules.tar.gz"
    SHA1 0e337607d4ce55cdc80c37b60dcfe4dbbf7812cf
    CMAKE_ARGS ENABLE_TESTING=OFF USE_THIRDPARTY_LIBRARIES=OFF OVERRIDE_CXX_STANDARD_FLAGS=OFF ENABLE_PULL=OFF ENABLE_PUSH=OFF ENABLE_COMPRESSION=OFF
    )
