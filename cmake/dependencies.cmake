# hunter dependencies
# https://docs.hunter.sh/en/latest/packages/

# https://docs.hunter.sh/en/latest/packages/pkg/GTest.html
hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/Boost.html
hunter_add_package(Boost)
find_package(Boost CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/libjson-rpc-cpp.html
hunter_add_package(libjson-rpc-cpp)
find_package(libjson-rpc-cpp CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/leveldb.html
hunter_add_package(leveldb)
find_package(leveldb CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/xxhash.html
hunter_add_package(xxhash)
find_package(xxhash CONFIG REQUIRED)

# other dependencies
include(ExternalProject)

find_package(binaryen)
