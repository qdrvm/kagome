## Template for add custom hunter config
#hunter_config(
#    package-name
#    VERSION 0.0.0-package-version
#    CMAKE_ARGS "CMAKE_VARIABLE=value"
#)

hunter_config(libp2p
    URL https://github.com/soramitsu/cpp-libp2p/archive/f075370398d3ddd444351e114d9c261cd3af244f.zip
    SHA1 322fc237a79292c03d2821c1338b83173e65be3f
    CMAKE_ARGS TESTING=OFF
    )
