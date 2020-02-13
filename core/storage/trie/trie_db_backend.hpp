/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_DB_BACKEND_HPP
#define KAGOME_TRIE_DB_BACKEND_HPP

#include <outcome/outcome.hpp>

#include "common/buffer.hpp"
#include "storage/buffer_map.hpp"
#include "storage/trie/impl/polkadot_node.hpp"

namespace kagome::storage::trie {

  /**
   * Adapter for key-value storages that allows to hide keyspace separation
   * along with root hash storing logic from the trie db component
   */
  class TrieDbBackend : public PersistentBufferMap {
   public:
    explicit TrieDbBackend(common::Buffer node_prefix)
        : node_prefix_{std::move(node_prefix)} {}
    ~TrieDbBackend() override = default;

    virtual outcome::result<void> saveRootHash(const common::Buffer &h) = 0;
    virtual outcome::result<common::Buffer> getRootHash() const = 0;

   protected:
    inline common::Buffer prefixKey(const common::Buffer &key) const {
      return common::Buffer{node_prefix_}.put(key);
    }

    inline common::Buffer getNodePrefix() const {
      return node_prefix_;
    }

   private:
    common::Buffer node_prefix_;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TRIE_DB_BACKEND_HPP
