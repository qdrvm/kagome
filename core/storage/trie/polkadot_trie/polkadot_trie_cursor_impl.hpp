/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_POLKADOT_TRIE_CURSOR_IMPL
#define KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_POLKADOT_TRIE_CURSOR_IMPL

#include "storage/trie/polkadot_trie/polkadot_trie_cursor.hpp"

#include "log/logger.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

namespace kagome::storage::trie {

  class PolkadotTrie;

  class PolkadotTrieCursorImpl : public PolkadotTrieCursor {
   public:
    using NodeType = PolkadotNode::Type;

    enum class Error {
      // operation cannot be performed for cursor position is not valid
      // due to an error, reaching the end or not calling next() after
      // initialization
      INVALID_NODE_TYPE = 1
    };

    explicit PolkadotTrieCursorImpl(const PolkadotTrie &trie);

    ~PolkadotTrieCursorImpl() override = default;

    static outcome::result<std::unique_ptr<PolkadotTrieCursorImpl>> createAt(
        const common::Buffer &key, const PolkadotTrie &trie);

    outcome::result<bool> seekFirst() override;

    outcome::result<bool> seek(const common::Buffer &key) override;

    outcome::result<bool> seekLast() override;

    /**
     * Seek the first element with key not less than \arg key
     */
    outcome::result<void> seekLowerBound(const common::Buffer &key) override;

    /**
     * Seek the first element with key greater than \arg key
     */
    outcome::result<void> seekUpperBound(const common::Buffer &key) override;

    bool isValid() const override;

    outcome::result<void> next() override;

    std::optional<common::Buffer> key() const override;

    std::optional<common::Buffer> value() const override;

   private:
    /**
     * An element of a path in trie. A node that is a part of the path and the
     * index of its child which is the next node in the path
     */
    struct TriePathEntry {
      TriePathEntry(Branch *parent, uint8_t child_idx)
          : parent{std::move(parent)}, child_idx{child_idx} {}

      Branch *parent;
      uint8_t child_idx;
    };

    outcome::result<void> seekLowerBoundInternal(
        Node *current, gsl::span<const uint8_t> left_nibbles);
    outcome::result<bool> seekNodeWithValueBothDirections();
    outcome::result<void> seekNodeWithValue(Node *node);
    outcome::result<bool> setChildWithMinIdx(Node *node, uint8_t min_idx = 0);
    /**
     * Constructs a list of branch nodes on the path from the root to the node
     * with the given \arg key
     */
    auto constructLastVisitedChildPath(const common::Buffer &key)
        -> outcome::result<std::list<TriePathEntry>>;

    template <typename Res, typename... Args>
    outcome::result<Res> safeAccess(outcome::result<Res> (*f)(Args...),
                                    Args &&...args) {
      auto res = f(std::forward<Args>(args)...);
      if (res.has_error()) {
        current_ = nullptr;
        search_state_.reset();
      }
    }

    common::Buffer collectKey() const;

    PolkadotCodec codec_;
    log::Logger log_;

    template <typename Res>
    outcome::result<Res> safeAccess(outcome::result<Res>&& value) {
      if (value.has_error()) {
        current_ = nullptr;
        search_state_.reset();
      }
      return value;
    }

    const PolkadotTrie &trie_;

#define SAFE_VOID_CALL(expr) \
  OUTCOME_TRY(safeAccess((expr)));

#define SAFE_CALL(res, expr) \
  OUTCOME_TRY(res, safeAccess((expr)));

    struct SearchState {
      std::list<TriePathEntry> last_visited_child;
      bool visited_root = false;

      void reset() {
        visited_root = false;
        last_visited_child.clear();
      }
    } search_state_;

    Node *current_;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, PolkadotTrieCursorImpl::Error);

#endif  // KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_POLKADOT_TRIE_CURSOR_IMPL
