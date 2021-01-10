## Template for add custom hunter config
#hunter_config(
#    package-name
#    VERSION 0.0.0-package-version
#    CMAKE_ARGS "CMAKE_VARIABLE=value"
#)

hunter_config(libp2p
    URL https://github.com/soramitsu/cpp-libp2p/archive/64a99dbe69f6a73536fc72dd044bf04fe2e416cb.zip
    SHA1 6501ce6cae5309a5597891b44d75de3ebabaf99e
    CMAKE_ARGS TESTING=OFF
    )
