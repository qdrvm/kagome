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
    return static_cast<int>(!value.empty()
                                ? PolkadotNode::Type::BranchWithValue
                                : PolkadotNode::Type::BranchEmptyValue);
  }

  uint16_t BranchNode::childrenBitmap() const {
    uint16_t bitmap = 0u;
    for (auto i = 0u; i < kMaxChildren; i++) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      if (children[i]) {
        bitmap = bitmap | 1u << i;
      }
    }
    return bitmap;
  }

  uint8_t BranchNode::childrenNum() const {
    uint8_t count = 0;
    for(auto &child: children) {
      if(child) {
        count++;
      }
    }
    return count;
  }

}  // namespace kagome::storage::merkle
