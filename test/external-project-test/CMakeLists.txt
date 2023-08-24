cmake_minimum_required(VERSION 3.12)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

set(
    CMAKE_TOOLCHAIN_FILE
    "../../cmake/toolchain/cxx17.cmake"
    CACHE
    FILEPATH
    "Default toolchain"
)

set(
    HUNTER_CACHE_SERVERS
    "https://github.com/soramitsu/hunter-binary-cache"
    CACHE
    STRING
    "Binary cache server"
)

include("cmake/HunterGate.cmake")
# todo: automatically update along with the kagome settings
include(../../cmake/Hunter/hunter-gate-url.cmake)

project(KagomeFromExternalProject)

hunter_add_package(kagome)
find_package(kagome CONFIG REQUIRED)

add_executable(main src/main.cpp)
include(link_libraries.cmake)
external_project_link_libraries(main kagome::)
