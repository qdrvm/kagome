/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_CURSOR_HPP
#define KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_CURSOR_HPP

#include "common/buffer.hpp"
#include "storage/face/map_cursor.hpp"
#include "storage/trie/impl/polkadot_node.hpp"
#include "storage/trie/impl/polkadot_trie.hpp"

namespace kagome::storage::trie {

  /**
   * @note Assumes no concurrent access to the trie!
   */
  class PolkadotTrieCursor
      : public face::MapCursor<common::Buffer, common::Buffer> {
   public:
    enum class Error {
      INVALID_CURSOR_POSITION =
          1,  // operation cannot be performed for cursor position is not valid
             // due to an error, reaching the end or not calling next() after
             // initialization
      NULL_ROOT // the root of the supplied trie is null
    };

    using NodePtr = std::shared_ptr<PolkadotNode>;
    using BranchPtr = std::shared_ptr<BranchNode>;
    using NodeType = PolkadotNode::Type;

    explicit PolkadotTrieCursor(const PolkadotTrie &trie);
    ~PolkadotTrieCursor() override = default;

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
    static int8_t hasNextChild(const BranchPtr &parent, uint8_t child_idx);

    static int8_t getPrevChildIdx(const BranchPtr &parent, uint8_t child_idx);
    static int8_t hasPrevChild(const BranchPtr &parent, uint8_t child_idx);

    // will either put a new entry or update the top entry (in case that parent
    // in the top entry is the same as \param parent
    void updateLastVisitedChild(const BranchPtr &parent, uint8_t child_idx);

    PolkadotCodec codec_;
    const PolkadotTrie &trie_;
    NodePtr current_;
    bool visited_root_ = false;
    std::list<std::pair<BranchPtr, int8_t>> last_visited_child_;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, PolkadotTrieCursor::Error);

#endif  // KAGOME_POLKADOT_TRIE_CURSOR_HPP
