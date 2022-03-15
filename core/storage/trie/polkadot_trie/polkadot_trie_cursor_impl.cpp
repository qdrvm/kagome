/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie/polkadot_trie_cursor_impl.hpp"

#include <iostream>

#include "common/buffer_back_insert_iterator.hpp"
#include "macro/unreachable.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie,
                            PolkadotTrieCursorImpl::Error,
                            e) {
  using E = kagome::storage::trie::PolkadotTrieCursorImpl::Error;
  switch (e) {
    case E::INVALID_NODE_TYPE:
      return "The processed node type is invalid";
    case E::METHOD_NOT_IMPLEMENTED:
      return "the method is not yet implemented";
  }
  return "unknown error";
}

namespace kagome::storage::trie {

  PolkadotTrieCursorImpl::PolkadotTrieCursorImpl(const PolkadotTrie &trie)
      : trie_{trie},
        current_{nullptr},
        log_{log::createLogger("TrieCursor", "trie", soralog::Level::TRACE)} {}

  outcome::result<std::unique_ptr<PolkadotTrieCursorImpl>>
  PolkadotTrieCursorImpl::createAt(const common::Buffer &key,
                                   const PolkadotTrie &trie) {
    auto c = std::make_unique<PolkadotTrieCursorImpl>(trie);
    OUTCOME_TRY(node,
                trie.getNode(trie.getRoot(), c->codec_.keyToNibbles(key)));
    c->visited_root_ = true;  // root is always visited first
    c->current_ = node;
    OUTCOME_TRY(last_child_path, c->constructLastVisitedChildPath(key));
    c->last_visited_child_ = std::move(last_child_path);

    return c;
  }

  outcome::result<bool> PolkadotTrieCursorImpl::seekFirst() {
    visited_root_ = false;
    last_visited_child_.clear();
    OUTCOME_TRY(next());
    return isValid();
  }

  outcome::result<bool> PolkadotTrieCursorImpl::seek(
      const common::Buffer &key) {
    if (trie_.getRoot() == nullptr) {
      current_ = nullptr;
      return false;
    }
    last_visited_child_.clear();
    visited_root_ = true;  // root is always visited first
    OUTCOME_TRY(last_child_path, constructLastVisitedChildPath(key));
    auto nibbles = PolkadotCodec::keyToNibbles(key);
    OUTCOME_TRY(node, trie_.getNode(trie_.getRoot(), nibbles));

    bool node_has_value = node != nullptr and node->value.has_value();
    if (node_has_value) {
      current_ = node;
    } else {
      current_ = nullptr;
      return false;
    }
    last_visited_child_ = std::move(last_child_path);
    return true;
  }

  outcome::result<bool> PolkadotTrieCursorImpl::seekLast() {
    NodePtr current = trie_.getRoot();
    if (current == nullptr) {
      current_ = nullptr;
      return false;
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
        // supposed to be unreachable
        return Error::INVALID_NODE_TYPE;
      }
    }
    current_ = current;
    return true;
  }

  outcome::result<void> PolkadotTrieCursorImpl::seekLowerBoundInternal(
      NodePtr current, gsl::span<const uint8_t> left_nibbles) {
    auto [left_mismatch, current_mismatch] =
        std::mismatch(left_nibbles.begin(),
                      left_nibbles.end(),
                      current->key_nibbles.begin(),
                      current->key_nibbles.end());
    // parts of every sequence within their common length (minimum of their
    // lengths) are equal
    bool part_equal = left_mismatch == left_nibbles.end()
                      or current_mismatch == current->key_nibbles.end();
    // if current choice is lexicographically less or equal to the left part
    // of the sought key, we just take the closest node with value
    bool less_or_eq =
        (part_equal and left_mismatch == left_nibbles.end())
        or (not part_equal and *left_mismatch < *current_mismatch);
    if (less_or_eq) {
      switch (current->getTrieType()) {
        case NodeType::BranchEmptyValue:
        case NodeType::BranchWithValue: {
          OUTCOME_TRY(seekNodeWithValue(current));
          current_ = current;
          return outcome::success();
        }
        case NodeType::Leaf:
          current_ = current;
          return outcome::success();
        default:
          return Error::INVALID_NODE_TYPE;
      }
    }
    // if the left part of the sought key exceeds the current node's key,
    // but its prefix is equal to the key, then we proceed to a child node
    // that starts with a nibble that is greater of equal to the first
    // nibble of the left part (if there is no such child, proceed to the
    // next case)
    bool longer = (part_equal and left_mismatch != left_nibbles.end()
                   and current_mismatch == current->key_nibbles.end());
    if (longer) {
      switch (current->getTrieType()) {
        case NodeType::BranchEmptyValue:
        case NodeType::BranchWithValue: {
          auto length = left_mismatch - left_nibbles.begin();
          auto branch = std::dynamic_pointer_cast<BranchNode>(current);
          OUTCOME_TRY(child, trie_.retrieveChild(branch, left_nibbles[length]));
          last_visited_child_.emplace_back(branch, left_nibbles[length]);
          if (child) {
            return seekLowerBoundInternal(child,
                                          left_nibbles.subspan(length + 1));
          }
          break;  // go to case3
        }
        case NodeType::Leaf: {
          break;  // go to case3
        }
        default:
          return Error::INVALID_NODE_TYPE;
      }
    }
    // if the left part of the key is longer than the current of
    // lexicographically greater than the current, we must return to its
    // parent and find a child greater than the current one
    bool longer_or_bigger =
        longer or (not part_equal and *left_mismatch > *current_mismatch);
    if (longer_or_bigger) {
      OUTCOME_TRY(ok, seekNodeWithValueBothDirections());
      if (not ok) {
        current_ = nullptr;
      }
      return outcome::success();
    }
    UNREACHABLE
  }

  outcome::result<void> PolkadotTrieCursorImpl::seekLowerBound(
      const common::Buffer &key) {
    if (trie_.getRoot() == nullptr) {
      current_ = nullptr;
      return outcome::success();
    }
    visited_root_ = true;
    last_visited_child_.clear();
    auto nibbles = PolkadotCodec::keyToNibbles(key);
    OUTCOME_TRY(seekLowerBoundInternal(trie_.getRoot(), nibbles));
    return outcome::success();
  }

  outcome::result<bool>
  PolkadotTrieCursorImpl::seekNodeWithValueBothDirections() {
    while (not last_visited_child_.empty()) {
      auto [parent, idx] = last_visited_child_.back();
      last_visited_child_.pop_back();
      auto parent_node = std::dynamic_pointer_cast<PolkadotNode>(parent);
      OUTCOME_TRY(ok, setChildWithMinIdx(parent_node, idx + 1));
      if (ok) {
        OUTCOME_TRY(seekNodeWithValue(parent_node));
        current_ = parent_node;
        return true;
      }
    }
    return false;
  }

  outcome::result<void> PolkadotTrieCursorImpl::seekNodeWithValue(
      NodePtr &parent) {
    if (parent->value.has_value()) {
      return outcome::success();
    }
    if (not parent->isBranch()) {
      return Error::INVALID_NODE_TYPE;
    }
    OUTCOME_TRY(setChildWithMinIdx(parent));
    OUTCOME_TRY(seekNodeWithValue(parent));
    return outcome::success();
  }

  outcome::result<void> PolkadotTrieCursorImpl::seekUpperBound(
      const common::Buffer &key) {
    OUTCOME_TRY(seekLowerBound(key));
    if (this->key().has_value() and this->key() == key) {
      OUTCOME_TRY(next());
    }
    return outcome::success();
  }

  outcome::result<bool> PolkadotTrieCursorImpl::setChildWithMinIdx(
      NodePtr &parent, uint8_t min_idx) {
    auto branch = std::dynamic_pointer_cast<BranchNode>(parent);
    for (uint8_t i = min_idx; i < BranchNode::kMaxChildren; i++) {
      if (branch->children.at(i)) {
        OUTCOME_TRY(child, trie_.retrieveChild(branch, i));
        last_visited_child_.emplace_back(branch, i);
        parent = child;
        return true;
      }
    }
    return false;
  }

  bool PolkadotTrieCursorImpl::isValid() const {
    return current_ != nullptr and current_->value.has_value();
  }

  outcome::result<void> PolkadotTrieCursorImpl::next() {
    if (not visited_root_) {
      current_ = trie_.getRoot();
    }
    // if we are in invalid position, next() just doesn't do anything
    if (current_ == nullptr) {
      return outcome::success();
    }

    if (not visited_root_ and trie_.getRoot()->value.has_value()) {
      visited_root_ = true;
      return outcome::success();
    }
    visited_root_ = true;
    if (current_->isBranch()) {
      OUTCOME_TRY(setChildWithMinIdx(current_));
      OUTCOME_TRY(seekNodeWithValue(current_));
      return outcome::success();
    }
    OUTCOME_TRY(ok, seekNodeWithValueBothDirections());
    if (not ok) {
      current_ = nullptr;
    }
    return outcome::success();
  }

  common::Buffer PolkadotTrieCursorImpl::collectKey() const {
    KeyNibbles key_nibbles;
    for (const auto &node_idx : last_visited_child_) {
      const auto &node = node_idx.parent;
      auto idx = node_idx.child_idx;
      std::copy(node->key_nibbles.begin(),
                node->key_nibbles.end(),
                std::back_inserter<Buffer>(key_nibbles));
      key_nibbles.putUint8(idx);
    }
    key_nibbles.put(current_->key_nibbles);
    using Codec = kagome::storage::trie::PolkadotCodec;
    return Codec::nibblesToKey(key_nibbles);
  }

  std::optional<common::Buffer> PolkadotTrieCursorImpl::key() const {
    if (current_ != nullptr) {
      return collectKey();
    }
    return std::nullopt;
  }

  std::optional<common::Buffer> PolkadotTrieCursorImpl::value() const {
    if (current_ != nullptr) {
      return current_->value.value();
    }
    return std::nullopt;
  }

  auto PolkadotTrieCursorImpl::constructLastVisitedChildPath(
      const common::Buffer &key) -> outcome::result<std::list<TriePathEntry>> {
    OUTCOME_TRY(path, trie_.getPath(trie_.getRoot(), codec_.keyToNibbles(key)));
    std::list<TriePathEntry> last_visited_child;
    for (auto &&[branch, idx] : path) {
      last_visited_child.emplace_back(branch, idx);
    }
    return last_visited_child;
  }

}  // namespace kagome::storage::trie
