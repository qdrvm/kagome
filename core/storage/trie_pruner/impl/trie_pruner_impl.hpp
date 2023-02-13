/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_PRUNER_IMPL_HPP
#define KAGOME_TRIE_PRUNER_IMPL_HPP

#include "storage/trie_pruner/trie_pruner.hpp"

#include <unordered_map>
#include <vector>

namespace kagome::storage::trie {
  class TrieStorageBackend;
}

namespace kagome::storage::trie_pruner {

  class StateWindow {
    std::vector<TrieStateUpdate> updates_;
    primitives::BlockNumber base_block_;
    std::unordered_map<common::Buffer, primitives::BlockNumber>
        block_where_node_was_removed_;
  };

  class TriePrunerImpl : public TriePruner {
   public:
    virtual void addNewState(TrieStateUpdate const &update) override {

    }

    virtual void prune(primitives::BlockNumber last_finalized) override {

    }

   private:
    common::Buffer getNodeKey(common::Buffer value_key) const {

    }

    StateWindow prune_window_;
    std::shared_ptr<storage::trie::TrieStorageBackend> storage_;
  };

}  // namespace kagome::storage::trie_pruner

#endif  // KAGOME_TRIE_PRUNER_IMPL_HPP
