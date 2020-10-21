## Template for add custom hunter config
#hunter_config(
#    package-name
#    VERSION 0.0.0-package-version
#    CMAKE_ARGS "CMAKE_VARIABLE=value"
#)

hunter_config(libp2p
    URL https://github.com/igor-egorov/cpp-libp2p/archive/8c9d43ef89bb9628a908ee8995007c36261d38f8.zip
    SHA1 de610e5297c4a8e88699a6fa5d868283824f1abd
    CMAKE_ARGS TESTING=OFF
    )

# TODO (yuraz): PRE-451 remove it after collecting libsecp256k1 into hunter config
hunter_config(libsecp256k1
    URL https://github.com/soramitsu/soramitsu-libsecp256k1/archive/c7630e1bac638c0f16ee66d4dce7b5c49eecbaa5.zip
    SHA1 179e316b0fe5150f1b05ca70ec2ac1490fe2cb3b
    CMAKE_ARGS SECP256K1_BUILD_TEST=OFF
    )
