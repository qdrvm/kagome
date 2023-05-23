/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_PRUNER_MOCK_HPP
#define KAGOME_TRIE_PRUNER_MOCK_HPP

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
                addNewChildState,
                (const storage::trie::RootHash &parent_root,
                 common::BufferView key,
                 const trie::PolkadotTrie &new_trie,
                 trie::StateVersion version),
                (override));

    MOCK_METHOD(outcome::result<void>,
                markAsChild,
                (Parent parent, const common::Buffer &key, Child child),
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
                restoreState,
                (const primitives::BlockHeader &base_block,
                 const blockchain::BlockTree &block_tree),
                (override));

    MOCK_METHOD(std::optional<primitives::BlockInfo>,
                getLastPrunedBlock,
                (),
                (const, override));

    MOCK_METHOD(std::optional<uint32_t>,
                getPruningDepth,
                (),
                (const, override));
  };

}  // namespace kagome::storage::trie_pruner

#endif  // KAGOME_TRIE_PRUNER_MOCK_HPP
