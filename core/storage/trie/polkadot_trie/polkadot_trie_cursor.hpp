/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_POLKADOT_TRIE_CURSOR
#define KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_POLKADOT_TRIE_CURSOR

#include "storage/buffer_map_types.hpp"

#include "common/buffer.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"

namespace kagome::storage::trie {

  class PolkadotTrieCursor : public BufferStorageCursor {
   public:
    virtual ~PolkadotTrieCursor() override = default;

    /**
     * Seek the first element with key not less than \arg key
     * @return true if the trie is not empty
     */
    virtual outcome::result<void> seekLowerBound(
        const common::BufferView &key) = 0;

    /**
     * Seek the first element with key greater than \arg key
     * @return true if the trie is not empty
     */
    virtual outcome::result<void> seekUpperBound(
        const common::BufferView &key) = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_POLKADOT_TRIE_CURSOR
