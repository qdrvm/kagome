/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/polkadot_trie_cursor.hpp"

#include "outcome/outcome.hpp"

namespace kagome::storage::trie {

  PolkadotTrieCursor::PolkadotTrieCursor(std::shared_ptr<PolkadotTrie> trie)
      : trie_{std::move(trie)}, current_{trie_->getRoot()} {
    BOOST_ASSERT(trie_ != nullptr);
  }

  void PolkadotTrieCursor::seekToFirst() {
    visited_root_ = false;
    current_ = trie_->root_;
    next();
  }

  void PolkadotTrieCursor::seek(const common::Buffer &key) {
    if (not trie_->getRoot()) {
      current_ = nullptr;
      return;
    }
    auto nibbles = PolkadotCodec::keyToNibbles(key);
    auto node = trie_->getNode(trie_->getRoot(), nibbles);
    // maybe need to refactor this condition
    if (node.has_value() and node.value()->value.has_value()) {
      current_ = node.value();

    } else {
      current_ = nullptr;
    }
  }

  void PolkadotTrieCursor::seekToLast() {
    NodePtr current = trie_->root_;
    if (current == nullptr) {
      current_ = nullptr;
      return;
    }

    // find the rightmost leaf
    while (current->getTrieType() != NodeType::Leaf) {
      auto type = current->getTrieType();
      if (type == NodeType::BranchEmptyValue
          or type == NodeType::BranchWithValue) {
        auto branch = std::dynamic_pointer_cast<BranchNode>(current);
        // find the rightmost child
        for (int8_t i = branch->kMaxChildren - 1; i >= 0; i--) {
          if (branch->getChild(i) != nullptr) {
            current = branch->getChild(i);
          }
        }

      } else {
        current_ = nullptr;
        return;
      }
    }
    current_ = current;
  }

  bool PolkadotTrieCursor::isValid() const {
    return current_ != nullptr;
  }

  void PolkadotTrieCursor::next() {
    if (current_ == nullptr) return;
    if (not visited_root_ and trie_->getRoot()->value.has_value()) {
      current_ = trie_->getRoot();
      visited_root_ = true;
    }
    do {
      if (current_->getTrieType() == NodeType::Leaf) {
        if (last_visited_child_.empty()) {
          current_ = nullptr;
          return;
        }
        // assert last_visited_child_.back() == current.parent
        auto p = last_visited_child_.back().first;  // self.current.parent
        while (not hasNextChild(p, last_visited_child_.back().second)) {
          last_visited_child_.pop_back();
          if (last_visited_child_.empty()) {
            current_ = nullptr;
            return;
          }
          p = last_visited_child_.back().first;  // p.parent
        }
        auto i = getNextChildIdx(p, last_visited_child_.back().second);
        current_ = p->getChild(i);
        updateLastVisitedChild(p, i);

      } else if (current_->getTrieType() == NodeType::BranchEmptyValue
                 or current_->getTrieType() == NodeType::BranchWithValue) {
        auto p = std::dynamic_pointer_cast<BranchNode>(current_);
        if (last_visited_child_.empty()
            or last_visited_child_.back().first != p) {
          last_visited_child_.emplace_back(std::make_pair(p, -1));
        }
        while (not hasNextChild(p, last_visited_child_.back().second)) {
          last_visited_child_.pop_back();
          p = last_visited_child_.back().first;  // p.parent
        }
        auto i = getNextChildIdx(p, last_visited_child_.back().second);
        current_ = p->getChild(i);
        updateLastVisitedChild(p, i);
      }
    } while (not current_->value.has_value());
  }

  void PolkadotTrieCursor::prev() {
    BOOST_ASSERT_MSG(false, "Not implemented");
  }

  outcome::result<common::Buffer> PolkadotTrieCursor::key() const {
    if (current_ != nullptr) {
      return codec_.collectKey(current_);
    }
  }

  outcome::result<common::Buffer> PolkadotTrieCursor::value() const {
    return current_->value.value();
  }

  int8_t PolkadotTrieCursor::getNextChildIdx(const BranchPtr &parent,
                                             uint8_t child_idx) {
    for (uint8_t i = child_idx + 1; i < parent->kMaxChildren; i++) {
      if (parent->getChild(i) != nullptr) {
        return i;
      }
    }
    return -1;
  }

  int8_t PolkadotTrieCursor::hasNextChild(const BranchPtr &parent,
                                          uint8_t child_idx) {
    return getNextChildIdx(parent, child_idx) != -1;
  }

  int8_t PolkadotTrieCursor::getPrevChildIdx(const BranchPtr &parent,
                                             uint8_t child_idx) {
    for (int8_t i = child_idx - 1; i >= 0; i--) {
      if (parent->getChild(i) != nullptr) {
        return i;
      }
    }
    return -1;
  }

  int8_t PolkadotTrieCursor::hasPrevChild(const BranchPtr &parent,
                                          uint8_t child_idx) {
    return getNextChildIdx(parent, child_idx) != -1;
  }

  void PolkadotTrieCursor::updateLastVisitedChild(const BranchPtr &parent,
                                                  uint8_t child_idx) {
    if (last_visited_child_.back().first == parent) {
      last_visited_child_.pop_back();
    }
    last_visited_child_.emplace_back(std::make_pair(parent, child_idx));
  }

}  // namespace kagome::storage::trie
