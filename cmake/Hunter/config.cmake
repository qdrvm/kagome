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

hunter_config(libp2p # New LibP2P
    URL  "https://github.com/libp2p/cpp-libp2p/archive/c76b5f091ef6f3da946509a4b1508f48ca2be2cb.tar.gz"
    SHA1 "e823fae731950333891c65096331b35e799e5ba0"
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF
    KEEP_PACKAGE_SOURCES
    )
#hunter_config(libp2p  # Old LibP2P
#    URL  "https://github.com/libp2p/cpp-libp2p/archive/f6104193ad01bf8caef89fd6a15226f968dfbac7.tar.gz"
#    SHA1 "9cb2607146294573df548e569ce79641597564e9"
#    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF DUMMY=1
#    KEEP_PACKAGE_SOURCES
#    )

hunter_config(soralog
    URL  "https://github.com/soramitsu/soralog/archive/v0.0.5.tar.gz"
    SHA1 "8b192d0e4cced8743b8a5e943cd0a89a5ad7939d"
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF
    KEEP_PACKAGE_SOURCES
    )

hunter_config(Boost.DI
    URL  "https://github.com/xDimon/di/archive/probe.tar.gz"
    SHA1 "aa7985cf30cf3d73731a2341ada6a2b83254656c"
    CMAKE_ARGS BOOST_DI_OPT_BUILD_TESTS=OFF BOOST_DI_OPT_BUILD_EXAMPLES=OFF
    KEEP_PACKAGE_SOURCES
    )

