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

  class TriePruner {
   public:
    virtual ~TriePruner() = default;

    virtual outcome::result<void> addNewState(
        trie::PolkadotTrie const &new_trie, trie::StateVersion version) = 0;

    virtual outcome::result<void> addNewChildState(
        storage::trie::RootHash const &parent_root,
        trie::PolkadotTrie const &new_trie,
        trie::StateVersion version) = 0;

    // to avoid confusing parameter order
    struct Parent {
      explicit Parent(storage::trie::RootHash const &hash): hash{hash} {}
      storage::trie::RootHash const &hash;
    };
    struct Child {
      explicit Child(storage::trie::RootHash const &hash): hash{hash} {}
      storage::trie::RootHash const &hash;
    };
    virtual outcome::result<void> markAsChild(
        Parent parent,
        Child child) = 0;

    virtual outcome::result<void> pruneFinalized(
        primitives::BlockHeader const &state,
        primitives::BlockInfo const &next_block) = 0;

    virtual outcome::result<void> pruneDiscarded(
        primitives::BlockHeader const &state) = 0;
  };

}  // namespace kagome::storage::trie_pruner

#endif  // KAGOME_TRIEPRUNER_HPP
