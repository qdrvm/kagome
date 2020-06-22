# hunter dependencies
# https://docs.hunter.sh/en/latest/packages/

# https://docs.hunter.sh/en/latest/packages/pkg/GTest.html
hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)
find_package(GMock CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/Boost.html
hunter_add_package(Boost COMPONENTS random filesystem program_options)
find_package(Boost CONFIG REQUIRED  random filesystem program_options)

## TODO: uncomment when it is really needed
## https://docs.hunter.sh/en/latest/packages/pkg/libjson-rpc-cpp.html
#hunter_add_package(libjson-rpc-cpp)
#find_package(libjson-rpc-cpp CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/leveldb.html
hunter_add_package(leveldb)
find_package(leveldb CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/xxhash.html
hunter_add_package(xxhash)
find_package(xxhash CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/iroha-ed25519.html
hunter_add_package(iroha-ed25519)
find_package(ed25519 CONFIG REQUIRED)

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
#target_link_libraries(... RapidJSON::rapidjson)

# https://docs.hunter.sh/en/latest/packages/pkg/Microsoft.GSL.html
hunter_add_package(Microsoft.GSL)
find_package(Microsoft.GSL CONFIG REQUIRED)

# not in hunter, added in cmake/Hunter/config.cmake
hunter_add_package(sr25519)
find_package(sr25519 REQUIRED)

# not in hunter, added in cmake/Hunter/config.cmake
hunter_add_package(jsonrpc-lean)
find_package(jsonrpc-lean REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/spdlog.html
hunter_add_package(spdlog)
find_package(spdlog CONFIG REQUIRED)

# https://github.com/masterjedy/hat-trie
hunter_add_package(tsl_hat_trie)
find_package(tsl_hat_trie CONFIG REQUIRED)

# https://github.com/masterjedy/di
hunter_add_package(Boost.DI)
find_package(Boost.DI CONFIG REQUIRED)

# https://github.com/soramitsu/libp2p
hunter_add_package(libp2p)
find_package(libp2p CONFIG REQUIRED)

hunter_add_package(libsecp256k1)
find_package(libsecp256k1 CONFIG REQUIRED)
