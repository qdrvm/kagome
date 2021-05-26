/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie/polkadot_trie_cursor_impl.hpp"

#include "../../../../test/testutil/storage/polkadot_trie_printer.hpp"

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

  size_t getCommonLength(gsl::span<const uint8_t> first,
                         gsl::span<const uint8_t> second) {
    auto &&[left_nibbles_mismatch, current_nibbles_mismatch] =
        std::mismatch(first.begin(), first.end(), second.begin(), second.end());
    return left_nibbles_mismatch - first.begin();
  }

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
    current_ = trie_.getRoot();
    OUTCOME_TRY(next());
    return isValid();
  }

  outcome::result<bool> PolkadotTrieCursorImpl::seek(
      const common::Buffer &key) {
    if (trie_.getRoot() == nullptr) {
      current_ = nullptr;
      return false;
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

  outcome::result<void> PolkadotTrieCursorImpl::seekLowerBound(
      const common::Buffer &key) {
    if (trie_.getRoot() == nullptr) {
      current_ = nullptr;
      return outcome::success();
    }
    visited_root_ = true;
    auto nibbles = PolkadotCodec::keyToNibbles(key);
    gsl::span<const uint8_t> left_nibbles(nibbles);
    BOOST_ASSERT(left_nibbles.size() >= 0);
    NodePtr current = trie_.getRoot();
    while (true) {
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
            OUTCOME_TRY(node, seekNodeWithValue(current));
            current_ = node;
            return outcome::success();
          }
          case NodeType::Leaf:
            current_ = current;
            return outcome::success();
          default:
            return Error::INVALID_NODE_TYPE;
        }
      }
      // if the left part of the sought key exceeds the current node's key, but
      // its prefix is equal to the key, then we proceed to a child node that
      // starts with a nibble that is greater of equal to the first nibble of
      // the left part (if there is no such child, proceed to the next case)
      bool longer = (part_equal and left_mismatch != left_nibbles.end()
                     and current_mismatch == current->key_nibbles.end());
      if (longer) {
        switch (current->getTrieType()) {
          case NodeType::BranchEmptyValue:
          case NodeType::BranchWithValue: {
            auto current_as_branch =
                std::dynamic_pointer_cast<BranchNode>(current);
            OUTCOME_TRY(child_idx,
                        getChildWithMinIdx(current_as_branch, *left_mismatch));
            if (child_idx != -1) {
              last_visited_child_.emplace_back(current_as_branch, child_idx);
              OUTCOME_TRY(new_current,
                          trie_.retrieveChild(current_as_branch, child_idx));
              left_nibbles =
                  left_nibbles.subspan(current->key_nibbles.size() + 1);
              current = new_current;
              continue;
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
        while (not last_visited_child_.empty()) {
          auto [parent, idx] = last_visited_child_.back();
          last_visited_child_.pop_back();
          OUTCOME_TRY(child_idx, getChildWithMinIdx(parent, idx + 1));
          if (child_idx != -1) {
            OUTCOME_TRY(child, trie_.retrieveChild(parent, child_idx));
            OUTCOME_TRY(node, seekNodeWithValue(child));
            current_ = node;
            return outcome::success();
          }
        }
        current_ = nullptr;
        return outcome::success();
      }
    }
    UNREACHABLE
  }

  outcome::result<PolkadotTrieCursorImpl::NodePtr>
  PolkadotTrieCursorImpl::seekNodeWithValue(NodePtr node) {
    while (not node->value.has_value()) {
      if (not node->isBranch()) {
        return Error::INVALID_NODE_TYPE;  // can't be a leaf without a value
      }
      auto node_as_value = std::dynamic_pointer_cast<BranchNode>(node);
      for (uint8_t i = 0; i < BranchNode::kMaxChildren; i++) {
        OUTCOME_TRY(child, trie_.retrieveChild(node_as_value, i));
        if (child != nullptr) {
          last_visited_child_.emplace_back(node_as_value, i);
          switch (child->getTrieType()) {
            case NodeType::BranchEmptyValue:
              node = std::dynamic_pointer_cast<BranchNode>(child);
              goto BREAK;
            case NodeType::BranchWithValue:
            case NodeType::Leaf:
              return std::move(child);
            case NodeType::Special:
              return Error::INVALID_NODE_TYPE;
          }
        }
      }
    BREAK:;
    }
    return node;
  }

  outcome::result<void> PolkadotTrieCursorImpl::seekUpperBound(
      const common::Buffer &key) {
    OUTCOME_TRY(seekLowerBound(key));
    if (this->key().has_value() and this->key() == key) {
      OUTCOME_TRY(next());
    }
    return outcome::success();
  }

  outcome::result<int8_t> PolkadotTrieCursorImpl::getChildWithMinIdx(
      BranchPtr node, uint8_t min_idx) const {
    for (uint8_t i = min_idx; i < BranchNode::kMaxChildren; i++) {
      OUTCOME_TRY(child, trie_.retrieveChild(node, i));
      if (child) {
        return i;
      }
    }
    return -1;
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
          // indicate that we're going to descend to some child of the current
          // node
          last_visited_child_.emplace_back(p, UINT8_MAX);
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
        std::cout << p << " " << (int)i << " " << c << "\n";
      }
    } while (not current_->value.has_value());
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

  boost::optional<common::Buffer> PolkadotTrieCursorImpl::key() const {
    if (current_ != nullptr) {
      return collectKey();
    }
    return boost::none;
  }

  boost::optional<common::Buffer> PolkadotTrieCursorImpl::value() const {
    if (current_ != nullptr) {
      return current_->value.value();
    }
    return boost::none;
  }

  uint8_t PolkadotTrieCursorImpl::getNextChildIdx(const BranchPtr &parent,
                                                  uint8_t child_idx) {
    // if child_idx is UINT8_MAX then we're gonna return the first child
    for (uint8_t i = child_idx + 1; i < parent->kMaxChildren; i++) {
      if (parent->children.at(i) != nullptr) {
        return i;
      }
    }
    return UINT8_MAX;
  }

  bool PolkadotTrieCursorImpl::hasNextChild(const BranchPtr &parent,
                                            uint8_t child_idx) {
    return getNextChildIdx(parent, child_idx) != UINT8_MAX;
  }

  uint8_t PolkadotTrieCursorImpl::getPrevChildIdx(const BranchPtr &parent,
                                                  uint8_t child_idx) {
    if (child_idx == 0 || child_idx >= BranchNode::kMaxChildren)
      return UINT8_MAX;
    for (int8_t i = child_idx - 1; i >= 0; i--) {
      if (parent->children.at(i) != nullptr) {
        return i;
      }
    }
    return UINT8_MAX;
  }

  bool PolkadotTrieCursorImpl::hasPrevChild(const BranchPtr &parent,
                                            uint8_t child_idx) {
    return getPrevChildIdx(parent, child_idx) != UINT8_MAX;
  }

  void PolkadotTrieCursorImpl::updateLastVisitedChild(const BranchPtr &parent,
                                                      uint8_t child_idx) {
    if (last_visited_child_.back().parent == parent) {
      last_visited_child_.pop_back();
    }
    last_visited_child_.emplace_back(parent, child_idx);
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
