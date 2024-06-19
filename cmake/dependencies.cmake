# hunter dependencies
# https://docs.hunter.sh/en/latest/packages/

if (TESTING)
  # https://docs.hunter.sh/en/latest/packages/pkg/GTest.html
  hunter_add_package(GTest)
  find_package(GTest CONFIG REQUIRED)
endif ()

if (BACKWARD)
  hunter_add_package(backward-cpp)
  find_package(Backward)
endif ()

# https://docs.hunter.sh/en/latest/packages/pkg/Boost.html
hunter_add_package(Boost COMPONENTS random filesystem program_options date_time)
find_package(Boost CONFIG REQUIRED random filesystem program_options date_time)

hunter_add_package(qtils)
find_package(qtils CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/xxhash.html
hunter_add_package(xxhash)
find_library(XXHASH NAMES libxxhash.a REQUIRED PATHS "${XXHASH_ROOT}/lib")
add_library(xxhash::xxhash STATIC IMPORTED)
set_property(TARGET xxhash::xxhash PROPERTY IMPORTED_LOCATION "${XXHASH}")
target_include_directories(xxhash::xxhash INTERFACE "${XXHASH_ROOT}/include")

# https://docs.hunter.sh/en/latest/packages/pkg/binaryen.html
hunter_add_package(binaryen)
find_package(binaryen CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/benchmark.html
hunter_add_package(benchmark)
find_package(benchmark CONFIG REQUIRED)

# https://github.com/libp2p/cpp-libp2p
hunter_add_package(libp2p)
find_package(libp2p CONFIG REQUIRED)

# https://www.openssl.org/
hunter_add_package(BoringSSL)
find_package(OpenSSL CONFIG REQUIRED)

# https://developers.google.com/protocol-buffers/
hunter_add_package(Protobuf)
find_package(Protobuf CONFIG REQUIRED)

# http://rapidjson.org
hunter_add_package(RapidJSON)
find_package(RapidJSON CONFIG REQUIRED)

hunter_add_package(erasure_coding_crust)
find_package(erasure_coding_crust CONFIG REQUIRED)

hunter_add_package(schnorrkel_crust)
find_package(schnorrkel_crust CONFIG REQUIRED)

hunter_add_package(jsonrpc-lean)
find_package(jsonrpc-lean REQUIRED)

hunter_add_package(soralog)
find_package(soralog CONFIG REQUIRED)

# https://github.com/masterjedy/di
hunter_add_package(Boost.DI)
find_package(Boost.DI CONFIG REQUIRED)

# https://hunter.readthedocs.io/en/latest/packages/pkg/prometheus-cpp.html
hunter_add_package(prometheus-cpp)
find_package(prometheus-cpp CONFIG REQUIRED)

# https://github.com/qdrvm/soramitsu-libsecp256k1
hunter_add_package(libsecp256k1)
find_package(libsecp256k1 CONFIG REQUIRED)

hunter_add_package(scale)
find_package(scale CONFIG REQUIRED)

hunter_add_package(zstd)
find_package(zstd CONFIG REQUIRED)

  
if ("${WASM_COMPILER}" STREQUAL "WAVM")
  hunter_add_package(wavm)
  find_package(LLVM CONFIG REQUIRED)
  find_package(WAVM CONFIG REQUIRED)
endif ()

hunter_add_package(wabt)
find_package(wabt CONFIG REQUIRED)

hunter_add_package(zstd)
find_package(zstd CONFIG REQUIRED)

if ("${WASM_COMPILER}" STREQUAL "WasmEdge")
  hunter_add_package(WasmEdge)
  find_package(WasmEdge REQUIRED CONFIG)
endif ()

hunter_add_package(rocksdb)
find_package(RocksDB CONFIG REQUIRED)
