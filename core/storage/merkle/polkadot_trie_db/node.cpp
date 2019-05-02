/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/merkle/polkadot_trie_db/node.hpp"

namespace kagome::storage::merkle {

  Node::Type LeafNode::getType() const {
    return Type::Leaf;
  }

  Node::Type BranchNode::getType() const {
    return Type::Branch;
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
