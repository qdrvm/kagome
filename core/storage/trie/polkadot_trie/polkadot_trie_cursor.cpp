/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie/polkadot_trie_cursor.hpp"

#include "storage/trie/serialization/polkadot_codec.hpp"
#include "common/buffer_back_insert_iterator.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie,
                            PolkadotTrieCursor::Error,
                            e) {
  using E = kagome::storage::trie::PolkadotTrieCursor::Error;
  switch (e) {
    case E::INVALID_CURSOR_POSITION:
      return "operation cannot be performed; cursor position is invalid "
             "due to: an error or reaching the "
             "end or not calling next() after initialization";
    case E::NULL_ROOT:
      return "the root of the supplied trie is null";
    case E::METHOD_NOT_IMPLEMENTED:
      return "the method is not yet implemented";
  }
  return "unknown error";
}

namespace kagome::storage::trie {

  PolkadotTrieCursor::PolkadotTrieCursor(const PolkadotTrie &trie)
      : trie_{trie}, current_{nullptr} {}

  outcome::result<void> PolkadotTrieCursor::seekToFirst() {
    visited_root_ = false;
    current_ = trie_.getRoot();
    return next();
  }

  outcome::result<void> PolkadotTrieCursor::seek(const common::Buffer &key) {
    if (trie_.getRoot() == nullptr) {
      current_ = nullptr;
      return Error::NULL_ROOT;
    }
    auto nibbles = PolkadotCodec::keyToNibbles(key);
    auto node = trie_.getNode(trie_.getRoot(), nibbles);
    // maybe need to refactor this condition
    if (node.has_value() and node.value()->value.has_value()) {
      current_ = node.value();
    } else {
      current_ = nullptr;
      return Error::INVALID_CURSOR_POSITION;
    }
    return outcome::success();
  }

  outcome::result<void> PolkadotTrieCursor::seekToLast() {
    NodePtr current = trie_.getRoot();
    if (current == nullptr) {
      current_ = nullptr;
      return Error::NULL_ROOT;
    }

    // find the rightmost leaf
    while (current->getTrieType() != NodeType::Leaf) {
      auto type = current->getTrieType();
      if (type == NodeType::BranchEmptyValue
          or type == NodeType::BranchWithValue) {
        auto branch = std::dynamic_pointer_cast<BranchNode>(current);
        // find the rightmost child
        for (int8_t i = branch->kMaxChildren - 1; i >= 0; i--) {
          if (branch->children.at(i) != nullptr) {
            OUTCOME_TRY(c, trie_.retrieveChild(branch, i));
            current = c;
          }
        }

      } else {
        BOOST_ASSERT_MSG(
            false,
            "must not reach here, as with using retrieveChild there should "
            "appear no other trie node type than leaf and both branch types");
      }
    }
    current_ = current;
    return outcome::success();
  }

  bool PolkadotTrieCursor::isValid() const {
    return current_ != nullptr and current_->value.has_value();
  }

  outcome::result<void> PolkadotTrieCursor::next() {
    if (not visited_root_) {
      current_ = trie_.getRoot();
    }
    if (current_ == nullptr) {
      return trie_.getRoot() == nullptr ? Error::NULL_ROOT
                                        : Error::INVALID_CURSOR_POSITION;
    }
    if (not visited_root_ and trie_.getRoot()->value.has_value()) {
      visited_root_ = true;
      return outcome::success();
    }
    visited_root_ = true;
    do {
      if (current_->getTrieType() == NodeType::Leaf) {
        if (last_visited_child_.empty()) {
          current_ = nullptr;
          return outcome::success();
        }
        // assert last_visited_child_.back() == current.parent
        auto p = last_visited_child_.back().first;  // self.current.parent
        while (not hasNextChild(p, last_visited_child_.back().second)) {
          last_visited_child_.pop_back();
          if (last_visited_child_.empty()) {
            current_ = nullptr;
            return outcome::success();
          }
          p = last_visited_child_.back().first;  // p.parent
        }
        auto i = getNextChildIdx(p, last_visited_child_.back().second);
        OUTCOME_TRY(c, trie_.retrieveChild(p, i));
        current_ = c;
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
        OUTCOME_TRY(c, trie_.retrieveChild(p, i));
        current_ = c;
        updateLastVisitedChild(p, i);
      }
    } while (not current_->value.has_value());
    return outcome::success();
  }

  outcome::result<void> PolkadotTrieCursor::prev() {
    BOOST_ASSERT_MSG(false, "Not implemented yet");
    return Error::METHOD_NOT_IMPLEMENTED;
  }

  common::Buffer PolkadotTrieCursor::collectKey() const {
    common::Buffer key_nibbles;
    for (auto &node_idx : last_visited_child_) {
      auto &node = node_idx.first;
      auto idx = node_idx.second;
      std::copy(node->key_nibbles.begin(),
                node->key_nibbles.end(),
                std::back_inserter(key_nibbles));
      key_nibbles.putUint8(idx);
    }
    key_nibbles.put(current_->key_nibbles);
    return codec_.nibblesToKey(key_nibbles);
  }

  outcome::result<common::Buffer> PolkadotTrieCursor::key() const {
    if (current_ != nullptr) {
      return collectKey();
    }
    return Error::INVALID_CURSOR_POSITION;
  }

  outcome::result<common::Buffer> PolkadotTrieCursor::value() const {
    if (current_ != nullptr) {
      return current_->value.value();
    }
    return Error::INVALID_CURSOR_POSITION;
  }

  int8_t PolkadotTrieCursor::getNextChildIdx(const BranchPtr &parent,
                                             uint8_t child_idx) {
    for (uint8_t i = child_idx + 1; i < parent->kMaxChildren; i++) {
      if (parent->children.at(i) != nullptr) {
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
      if (parent->children.at(i) != nullptr) {
        return i;
      }
    }
    return -1;
  }

  int8_t PolkadotTrieCursor::hasPrevChild(const BranchPtr &parent,
                                          uint8_t child_idx) {
    return getPrevChildIdx(parent, child_idx) != -1;
  }

  void PolkadotTrieCursor::updateLastVisitedChild(const BranchPtr &parent,
                                                  uint8_t child_idx) {
    if (last_visited_child_.back().first == parent) {
      last_visited_child_.pop_back();
    }
    last_visited_child_.emplace_back(std::make_pair(parent, child_idx));
  }

}  // namespace kagome::storage::trie
