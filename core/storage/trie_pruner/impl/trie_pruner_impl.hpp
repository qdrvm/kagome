/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_PRUNER_IMPL_HPP
#define KAGOME_TRIE_PRUNER_IMPL_HPP

#include "storage/trie_pruner/trie_pruner.hpp"

#include <unordered_map>

namespace kagome::storage::trie_pruner {

  class StateWindow {
    std::vector<TrieStateUpdate> updates_;
    primitives::BlockNumber base_block_;
    std::unordered_map<common::Buffer, primitives::BlockNumber>
        node_removal_index;
  };

  class TriePrunerImpl : public TriePruner {};

}  // namespace kagome::storage::trie_pruner

#endif  // KAGOME_TRIE_PRUNER_IMPL_HPP
