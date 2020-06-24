## Template for add custom hunter config
#hunter_config(
#    package-name
#    VERSION 0.0.0-package-version
#    CMAKE_ARGS "CMAKE_VARIABLE=value"
#)

hunter_config(libp2p
    URL https://github.com/soramitsu/cpp-libp2p/archive/5903b162af0841fe1b5724deb8db67c3ce45cd0f.zip
    SHA1 dd67286f875a89996dad1a262aee66e93280d110
    CMAKE_ARGS TESTING=OFF
    )

# TODO (yuraz): PRE-451 remove it after collecting libsecp256k1 into hunter config
hunter_config(libsecp256k1
    URL https://github.com/soramitsu/soramitsu-libsecp256k1-copy/archive/c7630e1bac638c0f16ee66d4dce7b5c49eecbaa5.zip
    SHA1 1c89a164663a89308d3f31d6b0fb89931d413dcf
    CMAKE_ARGS SECP256K1_BUILD_TEST=OFF
    )
