/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIEPRUNER_HPP
#define KAGOME_TRIEPRUNER_HPP

#include <vector>

#include "common/buffer.hpp"
#include "primitives/block_id.hpp"
#include "storage/trie/types.hpp"

namespace kagome::storage::trie_pruner {

  struct TrieStateUpdate {
    // block that introduced this update
    primitives::BlockNumber finalized_block;

    std::vector<common::Buffer> inserted_keys;
    std::vector<common::Buffer> removed_keys;
  };

  class TriePruner {
   public:
    virtual ~TriePruner() = default;

    virtual void addNewState(TrieStateUpdate const& update) = 0;
    virtual void prune(primitives::BlockNumber last_finalized) = 0;
  };

}

#endif  // KAGOME_TRIEPRUNER_HPP
