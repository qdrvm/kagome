/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie/polkadot_node.hpp"

namespace kagome::storage::trie {

  int BranchNode::getType() const {
    return static_cast<int>(value ? PolkadotNode::Type::BranchWithValue
                                  : PolkadotNode::Type::BranchEmptyValue);
  }

  uint16_t BranchNode::childrenBitmap() const {
    uint16_t bitmap = 0u;
    for (auto i = 0u; i < kMaxChildren; i++) {
      if (children.at(i)) {
        bitmap = bitmap | 1u << i;
      }
    }
    return bitmap;
  }

  uint8_t BranchNode::childrenNum() const {
    return std::count_if(children.begin(),
                         children.end(),
                         [](auto const &child) { return child; });
    ;
  }

  int LeafNode::getType() const {
    return static_cast<int>(PolkadotNode::Type::Leaf);
  }

}  // namespace kagome::storage::trie
