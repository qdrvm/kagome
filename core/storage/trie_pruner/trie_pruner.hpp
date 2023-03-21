/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIEPRUNER_HPP
#define KAGOME_TRIEPRUNER_HPP

#include <vector>

#include "common/buffer.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
#include "storage/trie/types.hpp"

namespace kagome::storage::trie {
  class PolkadotTrie;
}

namespace kagome::storage::trie_pruner {

  struct TrieStateUpdate {
    // block that introduced this update
    primitives::BlockNumber finalized_block;
    storage::trie::RootHash old_storage_root;

    std::vector<common::Buffer> inserted_keys;
    std::vector<common::Buffer> removed_keys;
  };

  class TriePruner {
   public:
    virtual ~TriePruner() = default;

    virtual outcome::result<void> addNewState(
        trie::PolkadotTrie const &new_trie, trie::StateVersion version) = 0;

    virtual outcome::result<void> prune(primitives::BlockHeader const &state,
                                        trie::StateVersion version) = 0;
  };

}  // namespace kagome::storage::trie_pruner

#endif  // KAGOME_TRIEPRUNER_HPP
