# hunter dependencies
# https://docs.hunter.sh/en/latest/packages/

if (TESTING)
    # https://docs.hunter.sh/en/latest/packages/pkg/GTest.html
    hunter_add_package(GTest)
    find_package(GTest CONFIG REQUIRED)
    find_package(GMock CONFIG REQUIRED)
endif()

# https://docs.hunter.sh/en/latest/packages/pkg/Boost.html
hunter_add_package(Boost COMPONENTS random filesystem program_options)
find_package(Boost CONFIG REQUIRED random filesystem program_options)

# https://docs.hunter.sh/en/latest/packages/pkg/leveldb.html
hunter_add_package(leveldb)
find_package(leveldb CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/xxhash.html
hunter_add_package(xxhash)
find_package(xxhash CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/binaryen.html
hunter_add_package(binaryen)
find_package(binaryen CONFIG REQUIRED)

# https://www.openssl.org/
hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)

# https://developers.google.com/protocol-buffers/
hunter_add_package(Protobuf)
find_package(Protobuf CONFIG REQUIRED)

# http://rapidjson.org
hunter_add_package(RapidJSON)
find_package(RapidJSON CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/Microsoft.GSL.html
hunter_add_package(Microsoft.GSL)
find_package(Microsoft.GSL CONFIG REQUIRED)

hunter_add_package(schnorrkel_crust)
find_package(schnorrkel_crust CONFIG REQUIRED)

hunter_add_package(jsonrpc-lean)
find_package(jsonrpc-lean REQUIRED)

hunter_add_package(soralog)
find_package(soralog CONFIG REQUIRED)

# https://github.com/masterjedy/hat-trie
hunter_add_package(tsl_hat_trie)
find_package(tsl_hat_trie CONFIG REQUIRED)

# https://github.com/masterjedy/di
hunter_add_package(Boost.DI)
find_package(Boost.DI CONFIG REQUIRED)

hunter_add_package(c-ares)
find_package(c-ares CONFIG REQUIRED)

# https://hunter.readthedocs.io/en/latest/packages/pkg/prometheus-cpp.html
hunter_add_package(prometheus-cpp)
find_package(prometheus-cpp CONFIG REQUIRED)

# https://github.com/soramitsu/libp2p
hunter_add_package(libp2p)
find_package(libp2p CONFIG REQUIRED)

# https://github.com/soramitsu/soramitsu-libsecp256k1
hunter_add_package(libsecp256k1)
find_package(libsecp256k1 CONFIG REQUIRED)

hunter_add_package(fmt)
find_package(fmt CONFIG REQUIRED)

hunter_add_package(yaml-cpp)
find_package(yaml-cpp CONFIG REQUIRED)
