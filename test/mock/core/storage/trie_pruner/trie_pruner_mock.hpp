/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/trie_pruner/trie_pruner.hpp"

#include "gmock/gmock.h"

namespace kagome::storage::trie_pruner {

  class TriePrunerMock final : public TriePruner {
   public:
    MOCK_METHOD(outcome::result<void>,
                addNewState,
                (const storage::trie::RootHash &new_trie,
                 trie::StateVersion version),
                (override));

    MOCK_METHOD(outcome::result<void>,
                addNewState,
                (const trie::PolkadotTrie &new_trie,
                 trie::StateVersion version),
                (override));

    MOCK_METHOD(outcome::result<void>,
                pruneFinalized,
                (const primitives::BlockHeader &state),
                (override));

    MOCK_METHOD(outcome::result<void>,
                pruneDiscarded,
                (const primitives::BlockHeader &state),
                (override));

    MOCK_METHOD(outcome::result<void>,
                recoverState,
                (const blockchain::BlockTree &block_tree),
                (override));

    MOCK_METHOD(std::optional<primitives::BlockInfo>,
                getLastPrunedBlock,
                (),
                (const, override));

    MOCK_METHOD(std::optional<uint32_t>,
                getPruningDepth,
                (),
                (const, override));

    MOCK_METHOD(void, reload, (const blockchain::BlockTree &), (override));
  };

}  // namespace kagome::storage::trie_pruner
