#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_executable(trie_pruner_benchmark storage/trie_pruner_benchmark.cpp)
target_link_libraries(trie_pruner_benchmark 
    storage 
    benchmark::benchmark 
    GTest::gmock_main 
    hasher 
    log_configurator
)
target_include_directories(trie_pruner_benchmark PRIVATE "${CMAKE_SOURCE_DIR}/test")
