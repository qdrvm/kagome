/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "storage/rocksdb/rocksdb.hpp"
#include "storage/trie/trie_batches.hpp"

namespace kagome::storage::trie {

  inline outcome::result<void> updateDirectStorage(const trie::RootHash &root,
                                                   trie::TrieBatch &trie,
                                                   RocksDbSpace &direct_storage,
                                                   log::Logger &log) {
    OUTCOME_TRY(direct_storage.clear());
    auto batch = direct_storage.batch();
    size_t count{};
    auto checkpoint = std::chrono::steady_clock::now();
    auto cursor = trie.trieCursor();
    OUTCOME_TRY(cursor->seekFirst());
    BOOST_ASSERT(cursor->isValid());
    while (cursor->isValid()) {
      OUTCOME_TRY(batch->put(cursor->key().value(), cursor->value().value()));
      OUTCOME_TRY(cursor->next());
      ++count;
      auto now = std::chrono::steady_clock::now();
      if (now - checkpoint > std::chrono::seconds(1)) {
        log->debug(
            "Inserted {} keys into direct storage with root {}", count, root);
        checkpoint = now;
      }
    }
    log->debug("Inserted total of {} keys into direct storage with root {}",
               count,
               root);
    OUTCOME_TRY(batch->put(storage::trie::kLastCommittedHashKey, root));
    OUTCOME_TRY(batch->commit());
    return outcome::success();
  }
}  // namespace kagome::storage::trie
