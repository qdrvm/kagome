/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/polkadot_node.hpp"

namespace kagome::storage::trie {

  int BranchNode::getType() const {
    return static_cast<int>(value ? PolkadotNode::Type::BranchWithValue
                                  : PolkadotNode::Type::BranchEmptyValue);
  }

  uint16_t BranchNode::getChildrenBitmap() const {
    uint16_t bitmap = 0u;
    for (auto i = 0u; i < kMaxChildren; i++) {
      if (children_.at(i)) {
        bitmap = bitmap | 1u << i;
      }
    }
    return bitmap;
  }

  uint8_t BranchNode::getChildrenNum() const {
    return std::count_if(children_.begin(),
                         children_.end(),
                         [](auto const &child) { return child; });
  }

  auto BranchNode::getChildren() const
      -> const std::array<std::shared_ptr<PolkadotNode>, kMaxChildren> & {
    return children_;
  }

  void BranchNode::setChild(uint8_t idx, std::shared_ptr<PolkadotNode> child) {
    if(child != nullptr) {
      child->parent = shared_from_this();
    }
    children_.at(idx) = std::move(child);
  }

  std::shared_ptr<PolkadotNode> BranchNode::getChild(uint8_t idx) {
    return children_.at(idx);
  }

  std::shared_ptr<const PolkadotNode> BranchNode::getChild(uint8_t idx) const {
    return children_.at(idx);
  }

  int8_t BranchNode::getChildIdx(const std::shared_ptr<const PolkadotNode> &child) const {
    for (uint8_t i = 0; i < kMaxChildren; i++) {
      if (getChild(i) == child) {
        return i;
      }
    }
    return -1;
  }

  int LeafNode::getType() const {
    return static_cast<int>(PolkadotNode::Type::Leaf);
  }

}  // namespace kagome::storage::trie
