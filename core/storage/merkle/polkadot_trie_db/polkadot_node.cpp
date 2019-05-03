/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/merkle/polkadot_trie_db/polkadot_node.hpp"

namespace kagome::storage::merkle {

  int LeafNode::getType() const {
    return static_cast<int>(PolkadotNode::Type::Leaf);
  }

  int BranchNode::getType() const {
    return static_cast<int>(PolkadotNode::Type::Branch);
  }

  uint16_t BranchNode::childrenBitmap() const {
    uint16_t bitmap = 0u;
    for (auto i = 0u; i < kMaxChildren; i++) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      if (children[i] != nullptr) {
        bitmap = bitmap | 1u << i;
      }
    }
    return bitmap;
  }
}  // namespace kagome::storage::merkle
