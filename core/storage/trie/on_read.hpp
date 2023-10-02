/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_ON_READ_HPP
#define KAGOME_STORAGE_TRIE_ON_READ_HPP

#include <memory>
#include <unordered_map>

#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace kagome::storage::trie {
  /**
   * Records db reads required to prove operations on trie.
   */
  struct OnRead {
    /**
     * Make callback.
     * Not `operator()` to avoid copy.
     */
    auto onRead() {
      return [this](const common::Hash256 &hash, common::BufferView raw) {
        if (db.emplace(hash, raw).second) {
          size += raw.size();
        }
      };
    }

    /**
     * Return nodes, not compact encoding.
     * Used by state rpc and light client protocol.
     */
    auto vec() {
      std::vector<common::Buffer> vec;
      for (auto &p : db) {
        vec.emplace_back(std::move(p.second));
      }
      db.clear();
      return vec;
    }

    std::unordered_map<common::Hash256, common::Buffer> db;
    size_t size = 0;
  };
}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_ON_READ_HPP
