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
                (trie::PolkadotTrie const &new_trie,
                 trie::StateVersion version),
                (override));

    MOCK_METHOD(outcome::result<void>,
                addNewChildState,
                (storage::trie::RootHash const &parent_root,
                 common::BufferView key,
                 trie::PolkadotTrie const &new_trie,
                 trie::StateVersion version),
                (override));

    MOCK_METHOD(outcome::result<void>,
                markAsChild,
                (Parent parent, common::Buffer const &key, Child child),
                (override));

    MOCK_METHOD(outcome::result<void>,
                pruneFinalized,
                (primitives::BlockHeader const &state),
                (override));

    MOCK_METHOD(outcome::result<void>,
                pruneDiscarded,
                (primitives::BlockHeader const &state),
                (override));

    MOCK_METHOD(std::optional<primitives::BlockNumber>,
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
