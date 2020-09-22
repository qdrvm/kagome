/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_POLKADOT_TRIE_CURSOR
#define KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_POLKADOT_TRIE_CURSOR

#include "storage/face/map_cursor.hpp"

#include "common/buffer.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

namespace kagome::storage::trie {

  class PolkadotTrieCursor
      : public face::MapCursor<common::Buffer, common::Buffer> {
   public:
    using NodePtr = std::shared_ptr<PolkadotNode>;
    using BranchPtr = std::shared_ptr<BranchNode>;
    using NodeType = PolkadotNode::Type;

    enum class Error {
      INVALID_CURSOR_POSITION =
          1,  // operation cannot be performed for cursor position is not valid
      // due to an error, reaching the end or not calling next() after
      // initialization
      NOT_FOUND,  // the sought value is not found
      NULL_ROOT,  // the root of the supplied trie is null
      METHOD_NOT_IMPLEMENTED
    };

    explicit PolkadotTrieCursor(const PolkadotTrie &trie);
    ~PolkadotTrieCursor() override = default;

    static outcome::result<std::unique_ptr<PolkadotTrieCursor>> createAt(
        const common::Buffer &key, const PolkadotTrie &trie);

    outcome::result<void> seekToFirst() override;
    outcome::result<void> seek(const common::Buffer &key) override;
    outcome::result<void> seekToLast() override;
    bool isValid() const override;
    outcome::result<void> next() override;
    outcome::result<void> prev() override;
    outcome::result<common::Buffer> key() const override;
    outcome::result<common::Buffer> value() const override;

   private:
    static int8_t getNextChildIdx(const BranchPtr &parent, uint8_t child_idx);
    static bool hasNextChild(const BranchPtr &parent, uint8_t child_idx);

    static int8_t getPrevChildIdx(const BranchPtr &parent, uint8_t child_idx);
    static bool hasPrevChild(const BranchPtr &parent, uint8_t child_idx);

    // will either put a new entry or update the top entry (in case that parent
    // in the top entry is the same as \param parent
    void updateLastVisitedChild(const BranchPtr &parent, uint8_t child_idx);

    /**
     * An element of a path in trie. A node that is a part of the path and the
     * index of its child which is the next node in the path
     */
    struct TriePathEntry {
      TriePathEntry(BranchPtr parent, int8_t child_idx)
          : parent{std::move(parent)}, child_idx{child_idx} {}
      BranchPtr parent;
      int8_t child_idx;
    };

    /**
     * Constructs a list of branch nodes on the path from the root to the node
     * with the given \arg key
     */
    auto constructLastVisitedChildPath(const common::Buffer &key)
        -> outcome::result<std::list<TriePathEntry>>;

    common::Buffer collectKey() const;

    const PolkadotTrie &trie_;
    PolkadotCodec codec_;
    NodePtr current_;
    bool visited_root_ = false;
    std::list<TriePathEntry> last_visited_child_;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, PolkadotTrieCursor::Error);

#endif  // KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_POLKADOT_TRIE_CURSOR
