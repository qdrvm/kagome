/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_TRIE_BATCH
#define KAGOME_STORAGE_TRIE_IMPL_TRIE_BATCH

#include "storage/buffer_map_types.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_cursor.hpp"
#include "storage/trie/types.hpp"

namespace kagome::storage::trie {

  class TrieBatch : public BufferStorage {
   public:
    ~TrieBatch() override = default;

    std::unique_ptr<Cursor> cursor() final {
      return trieCursor();
    }

    virtual std::unique_ptr<PolkadotTrieCursor> trieCursor() = 0;

    /**
     * Finalize all changes to the trie. This may involve writing them
     * to the database or just calculating the final root.
     * @param version
     * @return hash of the merkle value of the root trie node
     */
    virtual outcome::result<RootHash> commit(StateVersion version) = 0;

    /**
     * Remove all trie entries which key begins with the supplied prefix
     */
    virtual outcome::result<std::tuple<bool, uint32_t>> clearPrefix(
        const BufferView &prefix,
        std::optional<uint64_t> limit = std::nullopt) = 0;

    virtual outcome::result<std::optional<std::shared_ptr<TrieBatch>>>
    createChildBatch(common::BufferView path) = 0;
  };

  class TopperTrieBatch;

  /**
   * A batch on top of another batch
   * Used for small amount of atomic changes, like applying an extrinsic
   */
  class TopperTrieBatch : public TrieBatch {
   public:
    /**
     * Writes changes to the parent batch
     */
    virtual outcome::result<void> writeBack() = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_TRIE_BATCH
