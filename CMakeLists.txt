cmake_minimum_required(VERSION 3.13)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

include("cmake/Hunter/init.cmake")
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.23.105.tar.gz"
    SHA1 "1c7aaffc90d4898f0b55d03b77deb890b2aa0a2b"
)

PROJECT(kagome C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

include(cmake/dependencies.cmake)
include(cmake/functions.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

include_directories(
  ${PROJECT_SOURCE_DIR}/core
  # add path to header only deps here
  deps/spdlog/include
  deps/RxCpp/Rx/v2/src
)

add_subdirectory(core)

enable_testing()
add_subdirectory(test)
