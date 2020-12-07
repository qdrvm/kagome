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

hunter_config(schnorrkel_crust
        URL https://github.com/iceseer/soramitsu-sr25519-crust/archive/48c1300d02dad5b26618b42923208f4abb7bada6.zip
        SHA1 d9ed32e74b1ab340264204277871d7abb383dcb7
        CMAKE_ARGS TESTING=OFF
        )

# TODO (yuraz): PRE-451 remove it after collecting libsecp256k1 into hunter config
hunter_config(libsecp256k1
    URL https://github.com/soramitsu/soramitsu-libsecp256k1/archive/c7630e1bac638c0f16ee66d4dce7b5c49eecbaa5.zip
    SHA1 179e316b0fe5150f1b05ca70ec2ac1490fe2cb3b
    CMAKE_ARGS SECP256K1_BUILD_TEST=OFF
    )
