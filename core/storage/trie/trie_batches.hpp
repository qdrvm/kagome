/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_TRIE_BATCH
#define KAGOME_STORAGE_TRIE_IMPL_TRIE_BATCH

#include "storage/buffer_map_types.hpp"

namespace kagome::storage::trie {

  class TrieBatch : public face::Readable<Buffer, Buffer>,
                    public face::Writeable<Buffer, Buffer>,
                    public face::Iterable<Buffer, Buffer> {
   public:
    ~TrieBatch() override = default;

    /**
     * Calculate the merkle trie root of a trie to which the batch belongs.
     * Includes changes pending in the batch.
     */
    virtual outcome::result<Buffer> calculateRoot() const = 0;

    /**
     * Remove all trie entries which key begins with the supplied prefix
     */
    virtual outcome::result<void> clearPrefix(const Buffer &prefix) = 0;
  };

  /**
   * A batch that grants access to the persistent trie storage.
   * All changes are contained in memory until commit() is called.
   */
  class PersistentTrieBatch : public TrieBatch {
   public:
    /**
     * Commits changes to persistent storage
     * @returns committed trie root
     */
    virtual outcome::result<Buffer> commit() = 0;
  };

  /**
   * Temporary in-memory trie build on top of the persistent one
   * All changes to it are simply discarded when the batch is destroyed
   */
  class EphemeralTrieBatch : public TrieBatch {

  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_TRIE_BATCH
