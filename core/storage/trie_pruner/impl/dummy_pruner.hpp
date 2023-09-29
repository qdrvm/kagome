/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/trie_pruner/trie_pruner.hpp"

namespace kagome::storage::trie_pruner {

  class DummyPruner : public TriePruner {
   public:
    outcome::result<void> addNewState(const trie::RootHash &,
                                      trie::StateVersion) override {
      return outcome::success();
    }

    outcome::result<void> addNewState(const trie::PolkadotTrie &,
                                      trie::StateVersion) override {
      return outcome::success();
    }

    outcome::result<void> pruneFinalized(
        const primitives::BlockHeader &) override {
      return outcome::success();
    }

    outcome::result<void> pruneDiscarded(
        const primitives::BlockHeader &) override {
      return outcome::success();
    }

    outcome::result<void> recoverState(const blockchain::BlockTree &) override {
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
