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
