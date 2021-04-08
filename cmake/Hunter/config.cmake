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
    URL  "https://github.com/igor-egorov/cpp-libp2p/archive/feature/loopbackstream.tar.gz"
    SHA1 "a1773faafdc7f4f957c2eea01a3162a019684b0a"
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF DUMMY=2
    )

hunter_config(soralog
    URL  "https://github.com/xDimon/soralog/archive/4ec007a2ee0b0eb8c506a7a8c127d215ab6e1273.tar.gz"
    SHA1 "468d446beb7d67c7ac32331c89da4b9479511d1c"
    CMAKE_ARGS TESTING=OFF EXAMPLES=OFF
    KEEP_PACKAGE_SOURCES
    )
