#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(trie_pruner_test
    trie_pruner_test.cpp
)
target_link_libraries(trie_pruner_test
    storage
    logger
    log_configurator
    hasher
    primitives
)
