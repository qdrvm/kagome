/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/trie/polkadot_trie/polkadot_trie_cursor.hpp"

#include "log/logger.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

namespace kagome::storage::trie {

  class PolkadotTrie;

  class PolkadotTrieCursorImpl : public PolkadotTrieCursor {
   public:
    enum class Error {
      // cursor stumbled upon a node with a type invalid in the given context
      // (e.g. a leaf node where a branch node should've been)
      INVALID_NODE_TYPE = 1,
      // operation cannot be performed for cursor position is not valid
      // due to an error, reaching the end or not calling next() after
      // initialization
      INVALID_CURSOR_ACCESS,
      KEY_NOT_FOUND
    };

    explicit PolkadotTrieCursorImpl(std::shared_ptr<const PolkadotTrie> trie);

    ~PolkadotTrieCursorImpl() override = default;

    [[nodiscard]] static outcome::result<
        std::unique_ptr<PolkadotTrieCursorImpl>>
    createAt(const common::BufferView &key,
             const std::shared_ptr<const PolkadotTrie> &trie);

    [[nodiscard]] outcome::result<bool> seekFirst() override;

    [[nodiscard]] outcome::result<bool> seek(
        const common::BufferView &key) override;

    [[nodiscard]] outcome::result<bool> seekLast() override;

    /**
     * Seek the first element with key not less than \arg key
     */
    [[nodiscard]] outcome::result<void> seekLowerBound(
        const common::BufferView &key) override;

    /**
     * Seek the first element with key greater than \arg key
     */
    [[nodiscard]] outcome::result<void> seekUpperBound(
        const common::BufferView &key) override;

    [[nodiscard]] bool isValid() const override;

    [[nodiscard]] outcome::result<void> next() override;

    [[nodiscard]] outcome::result<void> prev() override;

    [[nodiscard]] std::optional<common::Buffer> key() const override;

    [[nodiscard]] std::optional<BufferOrView> value() const override;

   private:
    outcome::result<void> seekLowerBoundInternal(const TrieNode &current,
                                                 BufferView left_nibbles);
    outcome::result<bool> nextNodeWithValueInOuterTree();
    outcome::result<void> nextNodeWithValueInSubTree(
        const TrieNode &subtree_root);
    outcome::result<const TrieNode *> visitChildWithMinIdx(const TrieNode &node,
                                                           uint8_t min_idx = 0);

    /**
     * An element of a path in trie. A node that is a part of the path and the
     * index of its child which is the next node in the path
     */
    struct TriePathEntry {
      TriePathEntry(const BranchNode &parent, uint8_t child_idx)
          : parent{parent}, child_idx{child_idx} {}

      const BranchNode &parent;
      uint8_t child_idx;
    };

    class SearchState {
     public:
      explicit SearchState(const TrieNode &root) : current_{&root} {}

      SearchState(SearchState &&state)
          : current_{state.current_}, path_{std::move(state.path_)} {}

      SearchState &operator=(SearchState &&state) {
        current_ = state.current_;
        path_ = std::move(state.path_);
        return *this;
      }

      SearchState(const SearchState &state) = delete;
      SearchState &operator=(const SearchState &state) = delete;

      ~SearchState() = default;

      // need to pass child because of DummyNode logic (cannot obtain child
      // directly, need to call retrieveChild of PolkadotTrie)
      [[nodiscard]] outcome::result<void> visitChild(uint8_t index,
                                                     const TrieNode &child);

      [[nodiscard]] bool leaveChild() {
        if (isAtRoot()) {
          return false;
        }
        current_ = &path_.back().parent;
        path_.pop_back();
        return true;
      }

      bool isAtRoot() const {
        return path_.empty();
      }

      const TrieNode &getCurrent() const {
        BOOST_ASSERT_MSG(current_ != nullptr,
                         "SearchState implementation should guarantee it");
        return *current_;
      }

      const std::vector<TriePathEntry> &getPath() const {
        return path_;
      }

     private:
      const TrieNode *current_;
      std::vector<TriePathEntry> path_;  // from root to current
    };

    // cursor state was invalidated and not restored
    struct InvalidState {
      std::error_code code;
    };

    // cursor was created but no seek was performed
    struct UninitializedState {};

    struct ReachedEndState {};

    /**
     * Constructs a list of branch nodes on the path from the root to the node
     * with the given \arg key
     */
    auto makeSearchStateAt(const common::BufferView &key)
        -> outcome::result<SearchState>;

    common::Buffer collectKey() const;

    PolkadotCodec codec_;
    log::Logger log_;

    template <typename Res>
    outcome::result<Res> safeAccess(outcome::result<Res> &&result) {
      if (result.has_error()) {
        state_ = InvalidState{result.error()};
      }
      return std::move(result);
    }
#define SAFE_VOID_CALL(expr) OUTCOME_TRY(safeAccess((expr)));

#define SAFE_CALL(res, expr) OUTCOME_TRY(res, safeAccess((expr)));

    std::shared_ptr<const PolkadotTrie> trie_;

    using CursorState = std::
        variant<UninitializedState, SearchState, InvalidState, ReachedEndState>;
    CursorState state_;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, PolkadotTrieCursorImpl::Error)
