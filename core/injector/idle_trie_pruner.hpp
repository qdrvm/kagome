/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/trie_pruner/trie_pruner.hpp"

namespace kagome::storage::trie_pruner {

  /**
   * IdleTriePruner does nothing, so it can be used to disabled pruning
   */
  class IdleTriePruner : public storage::trie_pruner::TriePruner {
   public:
    outcome::result<void> addNewState(
        const storage::trie::RootHash &state_root,
        storage::trie::StateVersion version) override {
      return outcome::success();
    }

    outcome::result<void> addNewState(
        const storage::trie::PolkadotTrie &new_trie,
        storage::trie::StateVersion version) override {
      return outcome::success();
    }

    outcome::result<void> pruneFinalized(
        const primitives::BlockHeader &state) override {
      return outcome::success();
    }

    outcome::result<void> pruneDiscarded(
        const primitives::BlockHeader &state) override {
      return outcome::success();
    }

    outcome::result<void> recoverState(
        const blockchain::BlockTree &block_tree) override {
      return outcome::success();
    }

    std::optional<primitives::BlockInfo> getLastPrunedBlock() const override {
      return std::nullopt;
    }

    std::optional<uint32_t> getPruningDepth() const override {
      return std::nullopt;
    }
  };
}  // namespace kagome::storage::trie_pruner
