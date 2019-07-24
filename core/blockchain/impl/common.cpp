/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/common.hpp"

#include "blockchain/impl/level_db_util.hpp"
#include "common/visitor.hpp"
#include "storage/leveldb/leveldb_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain, Error, e) {
  switch (e) {
    case kagome::blockchain::Error::NOT_FOUND:
      return "A header with the provided id is not found";
  }
  return "Unknown error";
}

namespace kagome::blockchain {
  using kagome::common::Buffer;

  outcome::result<common::Buffer> idToLookupKey(const ReadableBufferMap &db,
                                                const primitives::BlockId &id) {
    auto key = visit_in_place(
        id,
        [&db](const primitives::BlockNumber &n) {
          auto key = prependPrefix(numberToIndexKey(n),
                                   prefix::Prefix::ID_TO_LOOKUP_KEY);
          return db.get(key);
        },
        [&db](const common::Hash256 &hash) {
          return db.get(
              prependPrefix(Buffer{hash}, prefix::Prefix::ID_TO_LOOKUP_KEY));
        });
    if (!key && key.error() == storage::LevelDBError::NOT_FOUND) {
      return Error::NOT_FOUND;
    }
    return key;
  }
}  // namespace kagome::blockchain
