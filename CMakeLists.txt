cmake_minimum_required(VERSION 3.13)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

include("cmake/Hunter/init.cmake")
HunterGate(
  URL "https://github.com/ruslo/hunter/archive/v0.23.89.tar.gz"
  SHA1 "a370290a8b32987755a36d64807bf8c7f5b61730"
)

PROJECT(kagome C CXX)

include(cmake/dependencies.cmake)
include(cmake/functions.cmake)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

include_directories(
  # add path to header only deps here
  deps/spdlog/include
)

add_subdirectory(core)

enable_testing()
add_subdirectory(test)
