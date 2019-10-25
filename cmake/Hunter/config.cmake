hunter_config(
    Boost
    VERSION 1.70.0-p0
    CMAKE_ARGS CMAKE_POSITION_INDEPENDENT_CODE=ON
)

hunter_config(
    GTest
    VERSION 1.8.0-hunter-p11
    CMAKE_ARGS "CMAKE_CXX_FLAGS=-Wno-deprecated-copy"
)

hunter_config(sr25519
    URL https://github.com/Warchant/sr25519-crust/archive/1.0.1.tar.gz
    SHA1 3005d79b23b92ff27848c24a7751543a03a2dd13
    )

hunter_config(libp2p
    URL https://github.com/soramitsu/libp2p/archive/feature/hide-outcome.zip
    SHA1 fe3d81e35442d9d21ae7afc752dd5d999886499b
    )
