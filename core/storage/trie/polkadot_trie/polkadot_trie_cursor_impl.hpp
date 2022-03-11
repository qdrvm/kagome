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
      INVALID_NODE_TYPE = 1,
      METHOD_NOT_IMPLEMENTED
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
      TriePathEntry(BranchPtr parent, uint8_t child_idx)
          : parent{std::move(parent)}, child_idx{child_idx} {}

      BranchPtr parent;
      uint8_t child_idx;
    };

    // will either put a new entry or update the top entry (in case that parent
    // in the top entry is the same as \param parent
    void updateLastVisitedChild(const BranchPtr &parent, uint8_t child_idx);

    outcome::result<void> seekLowerBoundInternal(
        NodePtr current, gsl::span<const uint8_t> left_nibbles);
    outcome::result<bool> seekNodeWithValueBothDirections();
    outcome::result<void> seekNodeWithValue(NodePtr &node);
    outcome::result<bool> setChildWithMinIdx(NodePtr &node,
                                             uint8_t min_idx = 0);
    /**
     * Constructs a list of branch nodes on the path from the root to the node
     * with the given \arg key
     */
    auto constructLastVisitedChildPath(const common::Buffer &key)
        -> outcome::result<std::list<TriePathEntry>>;

    common::Buffer collectKey() const;

    PolkadotCodec codec_;
    std::list<TriePathEntry> last_visited_child_;
    const PolkadotTrie &trie_;
    NodePtr current_;
    bool visited_root_ = false;
    log::Logger log_;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, PolkadotTrieCursorImpl::Error);

#endif  // KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_POLKADOT_TRIE_CURSOR_IMPL
