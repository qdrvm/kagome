/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie/polkadot_trie_cursor.hpp"

#include "common/buffer_back_insert_iterator.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

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
    case E::NOT_FOUND:
      return "the sought value is not found";
    case E::METHOD_NOT_IMPLEMENTED:
      return "the method is not yet implemented";
  }
  return "unknown error";
}

namespace kagome::storage::trie {

  PolkadotTrieCursor::PolkadotTrieCursor(const PolkadotTrie &trie)
      : trie_{trie}, current_{nullptr} {}

  outcome::result<std::unique_ptr<PolkadotTrieCursor>>
  PolkadotTrieCursor::createAt(const common::Buffer &key, const PolkadotTrie &trie) {
    auto c = std::make_unique<PolkadotTrieCursor>(trie);
    OUTCOME_TRY(node,
                trie.getNode(trie.getRoot(), c->codec_.keyToNibbles(key)));
    c->visited_root_ = true;  // root is always visited first
    c->current_ = node;
    OUTCOME_TRY(last_child_path, c->constructLastVisitedChildPath(key));
    c->last_visited_child_ = std::move(last_child_path);

    return c;
  }

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
    visited_root_ = true;  // root is always visited first
    OUTCOME_TRY(last_child_path, constructLastVisitedChildPath(key));
    auto nibbles = PolkadotCodec::keyToNibbles(key);
    OUTCOME_TRY(node, trie_.getNode(trie_.getRoot(), nibbles));

    bool node_has_value = node != nullptr and node->value.has_value();
    if (node_has_value) {
      current_ = node;
    } else {
      current_ = nullptr;
      return Error::NOT_FOUND;
    }
    last_visited_child_ = std::move(last_child_path);
    return outcome::success();
  }

  outcome::result<void> PolkadotTrieCursor::seekToLast() {
    NodePtr current = trie_.getRoot();
    if (current == nullptr) {
      current_ = nullptr;
      return Error::NULL_ROOT;
    }
    visited_root_ = true;  // root is always visited first
    last_visited_child_.clear();
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
            last_visited_child_.emplace_back(branch, i);
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
        auto p = last_visited_child_.back().parent;  // self.current.parent
        while (not hasNextChild(p, last_visited_child_.back().child_idx)) {
          last_visited_child_.pop_back();
          if (last_visited_child_.empty()) {
            current_ = nullptr;
            return outcome::success();
          }
          p = last_visited_child_.back().parent;  // p.parent
        }
        auto i = getNextChildIdx(p, last_visited_child_.back().child_idx);
        OUTCOME_TRY(c, trie_.retrieveChild(p, i));
        current_ = c;
        updateLastVisitedChild(p, i);

      } else if (current_->getTrieType() == NodeType::BranchEmptyValue
                 or current_->getTrieType() == NodeType::BranchWithValue) {
        auto p = std::dynamic_pointer_cast<BranchNode>(current_);
        if (last_visited_child_.empty()
            or last_visited_child_.back().parent != p) {
          last_visited_child_.emplace_back(p, -1);
        }
        while (not hasNextChild(p, last_visited_child_.back().child_idx)) {
          last_visited_child_.pop_back();
          if (last_visited_child_.empty()) {
            current_ = nullptr;
            return outcome::success();
          }
          p = last_visited_child_.back().parent;  // p.parent
        }
        auto i = getNextChildIdx(p, last_visited_child_.back().child_idx);
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
    KeyNibbles key_nibbles;
    for (auto &node_idx : last_visited_child_) {
      auto &node = node_idx.parent;
      auto idx = node_idx.child_idx;
      std::copy(node->key_nibbles.begin(),
                node->key_nibbles.end(),
                std::back_inserter<Buffer>(key_nibbles));
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

  bool PolkadotTrieCursor::hasNextChild(const BranchPtr &parent,
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

  bool PolkadotTrieCursor::hasPrevChild(const BranchPtr &parent,
                                          uint8_t child_idx) {
    return getPrevChildIdx(parent, child_idx) != -1;
  }

  void PolkadotTrieCursor::updateLastVisitedChild(const BranchPtr &parent,
                                                  uint8_t child_idx) {
    if (last_visited_child_.back().parent == parent) {
      last_visited_child_.pop_back();
    }
    last_visited_child_.emplace_back(parent, child_idx);
  }

  auto PolkadotTrieCursor::constructLastVisitedChildPath(
      const common::Buffer &key)
      -> outcome::result<
          std::list<TriePathEntry>> {
    OUTCOME_TRY(path, trie_.getPath(trie_.getRoot(), codec_.keyToNibbles(key)));
    std::list<TriePathEntry>
        last_visited_child;
    for (auto &&[branch, idx] : path) {
      last_visited_child.emplace_back(branch, idx);
    }
    return last_visited_child;
  }

}  // namespace kagome::storage::trie
