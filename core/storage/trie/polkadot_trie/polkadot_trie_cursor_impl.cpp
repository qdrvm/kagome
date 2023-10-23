/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie/polkadot_trie_cursor_impl.hpp"

#include <utility>

#include "common/buffer_back_insert_iterator.hpp"
#include "macro/unreachable.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie,
                            PolkadotTrieCursorImpl::Error,
                            e) {
  using E = kagome::storage::trie::PolkadotTrieCursorImpl::Error;
  switch (e) {
    case E::INVALID_NODE_TYPE:
      return "The processed node type is invalid";
    case E::INVALID_CURSOR_ACCESS:
      return "A trie cursor in an invalid state has been accessed (e.g. "
             "next())";
    case E::KEY_NOT_FOUND:
      return "The requested key was not found";
  }
  return "Unknown TrieCursor error";
}

namespace kagome::storage::trie {

  [[nodiscard]] outcome::result<void>
  PolkadotTrieCursorImpl::SearchState::visitChild(uint8_t index,
                                                  const TrieNode &child) {
    auto *current_as_branch = dynamic_cast<const BranchNode *>(current_);
    if (current_as_branch == nullptr) {
      return Error::INVALID_NODE_TYPE;
    }
    path_.emplace_back(*current_as_branch, index);
    current_ = &child;
    return outcome::success();
  }

  PolkadotTrieCursorImpl::PolkadotTrieCursorImpl(
      std::shared_ptr<const PolkadotTrie> trie)
      : log_{log::createLogger("TrieCursor", "trie")},
        trie_{std::move(trie)},
        state_{UninitializedState{}} {
    BOOST_ASSERT(trie_);
  }

  outcome::result<std::unique_ptr<PolkadotTrieCursorImpl>>
  PolkadotTrieCursorImpl::createAt(
      const common::BufferView &key,
      const std::shared_ptr<const PolkadotTrie> &trie) {
    auto cursor = std::make_unique<PolkadotTrieCursorImpl>(trie);
    OUTCOME_TRY(search_state, cursor->makeSearchStateAt(key));
    cursor->state_ = std::move(search_state);
    return cursor;
  }

  outcome::result<bool> PolkadotTrieCursorImpl::seekFirst() {
    state_ = UninitializedState{};
    SAFE_VOID_CALL(next())
    return isValid();
  }

  outcome::result<bool> PolkadotTrieCursorImpl::seek(
      const common::BufferView &key) {
    if (trie_->getRoot() == nullptr) {
      state_ = UninitializedState{};
      return false;
    }

    auto res = makeSearchStateAt(key);
    if (res.has_error()) {
      // if the given key is just not present in the trie, return false
      state_ = InvalidState{res.error()};
      if (res.error() == Error::KEY_NOT_FOUND) {
        return false;
      }
      // on other errors - propagate them
      return res.error();
    }
    state_ = std::move(res.value());
    auto &current = std::get<SearchState>(state_).getCurrent();
    // while there is a node in a trie with the given key, it contains no value,
    // thus cannot be pointed at by the cursor
    if (not current.getValue()) {
      OUTCOME_TRY(next());
    }
    return isValid();
  }

  outcome::result<bool> PolkadotTrieCursorImpl::seekLast() {
    auto *current = trie_->getRoot().get();
    if (current == nullptr) {
      state_ = UninitializedState{};
      return false;
    }
    state_ = SearchState{*current};
    auto &search_state = std::get<SearchState>(state_);

    // find the rightmost leaf
    while (current->isBranch()) {
      auto &branch = dynamic_cast<const BranchNode &>(*current);
      // find the rightmost child
      for (int8_t i = BranchNode::kMaxChildren - 1; i >= 0; i--) {
        if (branch.children.at(i) != nullptr) {
          SAFE_CALL(child, trie_->retrieveChild(branch, i))
          SAFE_VOID_CALL(search_state.visitChild(i, *child))
          current = &search_state.getCurrent();
          break;
        }
      }
    }
    return true;
  }

  outcome::result<void> PolkadotTrieCursorImpl::seekLowerBoundInternal(
      const TrieNode &current, gsl::span<const uint8_t> sought_nibbles) {
    BOOST_ASSERT(isValid());
    auto [sought_nibbles_mismatch, current_mismatch] =
        std::mismatch(sought_nibbles.begin(),
                      sought_nibbles.end(),
                      current.getKeyNibbles().begin(),
                      current.getKeyNibbles().end());
    // one nibble sequence is a prefix of the other
    bool sought_is_prefix = sought_nibbles_mismatch == sought_nibbles.end();
    bool current_is_prefix = current_mismatch == current.getKeyNibbles().end();

    // if sought nibbles are lexicographically less or equal to the current
    // nibbles, we just take the closest node with value
    bool sought_less_or_eq = sought_is_prefix
                          or (not current_is_prefix
                              and *sought_nibbles_mismatch < *current_mismatch);
    SL_TRACE(log_,
             "The sought key '{}' is {} than current '{}'",
             common::hex_lower(sought_nibbles),
             sought_less_or_eq ? "less or eq" : "greater",
             common::hex_lower(current.getKeyNibbles()));
    if (sought_less_or_eq) {
      if (current.isBranch()) {
        SL_TRACE(log_, "We're in a branch and search next node in subtree");
        SAFE_VOID_CALL(nextNodeWithValueInSubTree(current))
      } else {
        SL_TRACE(log_, "We're in a leaf {} and done", key().value());
      }
      return outcome::success();
    }
    // if the left part of the sought key exceeds the current node's key,
    // but its prefix is equal to the key, then we proceed to a child node
    // that starts with a nibble that is greater of equal to the first
    // nibble of the left part (if there is no such child, proceed to the
    // next case)
    bool sought_is_longer = current_is_prefix and not sought_is_prefix;
    if (sought_is_longer) {
      if (current.isBranch()) {
        auto mismatch_pos = sought_nibbles_mismatch - sought_nibbles.begin();
        auto &branch = dynamic_cast<const BranchNode &>(current);
        SAFE_CALL(child,
                  visitChildWithMinIdx(branch, sought_nibbles[mismatch_pos]))
        if (child) {
          uint8_t child_idx =
              std::get<SearchState>(state_).getPath().back().child_idx;
          SL_TRACE(log_,
                   "We're in a branch and proceed to child {:x}",
                   (int)child_idx);
          if (child_idx > sought_nibbles[mismatch_pos]) {
            return nextNodeWithValueInSubTree(*child);
          } else {
            return seekLowerBoundInternal(
                *child, sought_nibbles.subspan(mismatch_pos + 1));
          }
        }
      }
    }
    // if the left part of the key is longer than the current or
    // lexicographically greater than the current, we must return to its
    // parent and find a child greater than the current one
    bool longer_or_greater = sought_is_longer
                          or (not(sought_is_prefix or current_is_prefix)
                              and *sought_nibbles_mismatch > *current_mismatch);
    if (longer_or_greater) {
      SL_TRACE(log_, "We're looking for next node with value in outer tree");
      SAFE_CALL(found, nextNodeWithValueInOuterTree())
      if (!found) {
        state_ = ReachedEndState{};
      }
      SL_TRACE(log_, "Done at {}", key().value());
      return outcome::success();
    }
    UNREACHABLE
  }

  outcome::result<void> PolkadotTrieCursorImpl::seekLowerBound(
      const common::BufferView &key) {
    if (trie_->getRoot() == nullptr) {
      SL_TRACE(log_, "Seek lower bound for {} -> null root", key);
      state_ = UninitializedState{};
      return outcome::success();
    }
    state_ = SearchState{*trie_->getRoot()};
    auto nibbles = KeyNibbles::fromByteBuffer(key);
    SAFE_VOID_CALL(seekLowerBoundInternal(*trie_->getRoot(), nibbles))
    return outcome::success();
  }

  outcome::result<bool> PolkadotTrieCursorImpl::nextNodeWithValueInOuterTree() {
    BOOST_ASSERT(std::holds_alternative<SearchState>(state_));
    auto &search_state = std::get<SearchState>(state_);

    const auto &search_path = search_state.getPath();
    while (not search_path.empty()) {
      auto [parent, idx] = search_path.back();
      BOOST_VERIFY_MSG(search_state.leaveChild(),
                       "Guaranteed by the loop condition");
      SAFE_CALL(child, visitChildWithMinIdx(parent, idx + 1))
      if (child != nullptr) {
        SL_TRACE(log_,
                 "A greater child exists (idx {}), proceed to it",
                 search_path.back().child_idx);
        SAFE_VOID_CALL(nextNodeWithValueInSubTree(*child))
        return true;
      }
    }
    return false;
  }

  outcome::result<void> PolkadotTrieCursorImpl::nextNodeWithValueInSubTree(
      const TrieNode &parent) {
    auto *current = &parent;
    while (not current->getValue()) {
      if (not current->isBranch()) {
        return Error::INVALID_NODE_TYPE;
      }
      SAFE_CALL(child, visitChildWithMinIdx(*current))
      SL_TRACE(log_,
               "Proceed to child {:x}",
               (int)std::get<SearchState>(state_).getPath().back().child_idx);
      BOOST_ASSERT_MSG(child != nullptr, "Branch node must contain a leaf");
      current = child;
    }
    return outcome::success();
  }

  outcome::result<void> PolkadotTrieCursorImpl::seekUpperBound(
      const common::BufferView &sought_key) {
    SL_TRACE(log_, "Seek upper bound for {}", sought_key);
    SAFE_VOID_CALL(seekLowerBound(sought_key))
    if (auto key = this->key(); key.has_value() && key.value() == sought_key) {
      SAFE_VOID_CALL(next())
    }
    return outcome::success();
  }

  outcome::result<const TrieNode *>
  PolkadotTrieCursorImpl::visitChildWithMinIdx(const TrieNode &parent,
                                               uint8_t min_idx) {
    BOOST_ASSERT(std::holds_alternative<SearchState>(state_));
    auto &search_state = std::get<SearchState>(state_);
    for (uint8_t i = min_idx; i < BranchNode::kMaxChildren; i++) {
      auto &branch = dynamic_cast<const BranchNode &>(parent);
      if (branch.children.at(i)) {
        OUTCOME_TRY(child, trie_->retrieveChild(branch, i));
        BOOST_ASSERT(child != nullptr);
        OUTCOME_TRY(search_state.visitChild(i, *child));
        return child.get();
      }
    }
    return nullptr;
  }

  bool PolkadotTrieCursorImpl::isValid() const {
    return std::holds_alternative<SearchState>(state_);
  }

  outcome::result<void> PolkadotTrieCursorImpl::next() {
    if (std::holds_alternative<InvalidState>(state_)) {
      return Error::INVALID_CURSOR_ACCESS;
    }

    if (trie_->getRoot() == nullptr) {
      return outcome::success();
    }

    if (key().has_value()) {
      SL_TRACE(log_, "Searching next key, current is {}", key().value());
    } else {
      SL_TRACE(log_, "Searching next key, no current key");
    }

    if (std::holds_alternative<UninitializedState>(state_)) {
      state_ = SearchState{*trie_->getRoot()};
      if (trie_->getRoot()->getValue()) {
        return outcome::success();
      }
    }
    BOOST_ASSERT(std::holds_alternative<SearchState>(state_));
    auto &search_state = std::get<SearchState>(state_);

    // if we're in a branch, means nodes in its subtree are not visited yet
    if (search_state.getCurrent().isBranch()) {
      SL_TRACE(log_, "We're in a branch and looking for next value in subtree");
      SAFE_CALL(child, visitChildWithMinIdx(search_state.getCurrent()))
      BOOST_ASSERT_MSG(child != nullptr,
                       "Since parent is branch, there must be a child");
      SL_TRACE(log_, "Go to child {}", search_state.getPath().back().child_idx);
      SAFE_VOID_CALL(nextNodeWithValueInSubTree(*child))
      SL_TRACE(log_, "Found {}", key().value());
      return outcome::success();
    }
    // we're in a leaf, should go up the tree and search there
    SL_TRACE(log_, "We're in a leaf and looking for next value in outer tree");
    SAFE_CALL(found, nextNodeWithValueInOuterTree())
    if (not found) {
      SL_TRACE(log_, "Not found anything");
      state_ = ReachedEndState{};
    } else {
      SL_TRACE(log_, "Found {}", key().value());
    }
    return outcome::success();
  }

  outcome::result<void> PolkadotTrieCursorImpl::prev() {
    throw std::logic_error{"PolkadotTrieCursorImpl::prev not implemented"};
  }

  common::Buffer PolkadotTrieCursorImpl::collectKey() const {
    BOOST_ASSERT(std::holds_alternative<SearchState>(state_));
    const auto &search_state = std::get<SearchState>(state_);
    KeyNibbles key_nibbles;
    for (const auto &node_idx : search_state.getPath()) {
      const auto &node = node_idx.parent;
      auto idx = node_idx.child_idx;
      std::copy(node.getKeyNibbles().begin(),
                node.getKeyNibbles().end(),
                std::back_inserter<Buffer>(key_nibbles));
      key_nibbles.putUint8(idx);
    }
    key_nibbles.put(search_state.getCurrent().getKeyNibbles());
    return key_nibbles.toByteBuffer();
  }

  std::optional<common::Buffer> PolkadotTrieCursorImpl::key() const {
    if (const auto *search_state = std::get_if<SearchState>(&state_);
        search_state != nullptr) {
      return collectKey();
    }
    return std::nullopt;
  }

  std::optional<BufferOrView> PolkadotTrieCursorImpl::value() const {
    if (const auto *search_state = std::get_if<SearchState>(&state_);
        search_state != nullptr) {
      const auto &value_opt = search_state->getCurrent().getValue();
      if (value_opt) {
        // TODO(turuslan): #1470, return error
        if (auto r =
                trie_->retrieveValue(const_cast<ValueAndHash &>(value_opt));
            !r) {
          SL_WARN(log_,
                  "PolkadotTrieCursorImpl::value {}: {}",
                  common::hex_lower_0x(collectKey()),
                  r.error());
          return std::nullopt;
        }
        return BufferView{*value_opt.value};
      }
      return std::nullopt;
    }
    return std::nullopt;
  }

  auto PolkadotTrieCursorImpl::makeSearchStateAt(const common::BufferView &key)
      -> outcome::result<SearchState> {
    if (trie_->getRoot() == nullptr) {
      return Error::KEY_NOT_FOUND;
    }
    SearchState search_state{*trie_->getRoot()};

    auto add_visited_child = [&search_state](
                                 auto &branch,
                                 auto idx,
                                 auto &child) mutable -> outcome::result<void> {
      BOOST_VERIFY(  // NOLINT(bugprone-lambda-function-name)
          search_state.visitChild(idx, child));
      return outcome::success();
    };
    auto res = trie_->forNodeInPath(
        trie_->getRoot(), KeyNibbles::fromByteBuffer(key), add_visited_child);
    if (res.has_error()) {
      if (res.error() == TrieError::NO_VALUE) {
        return Error::KEY_NOT_FOUND;
      }
      return res.error();
    }
    return search_state;
  }

}  // namespace kagome::storage::trie
