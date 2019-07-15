/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/level_db_util.hpp"

using kagome::blockchain::prefix::Prefix;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain, LevelDbRepositoryError, e) {
  using E = kagome::blockchain::LevelDbRepositoryError;
  switch (e) {
    case E::INVALID_KEY:
      return "Invalid storage key";
  }
  return "Unknown error";
}

namespace kagome::blockchain {

  common::Buffer numberToIndexKey(primitives::BlockNumber n) {
    // TODO(Harrm) Figure out why exactly it is this way in substrate
    BOOST_ASSERT((n & 0xffffffff00000000) == 0);

    return {prefix::ID_TO_LOOKUP_KEY, uint8_t(n >> 24),
            uint8_t((n >> 16) & 0xff), uint8_t((n >> 8) & 0xff),
            uint8_t(n & 0xff)};
  }

  common::Buffer numberAndHashToLookupKey(primitives::BlockNumber number,
                                          const common::Hash256 &hash) {
    auto lookup_key = numberToIndexKey(number);
    lookup_key.put(hash);
    return lookup_key;
  }

  outcome::result<primitives::BlockNumber> lookupKeyToNumber(
      const common::Buffer &key) {
    if (key.size() < 4) {
      return LevelDbRepositoryError::INVALID_KEY;
    }
    return uint64_t(key[0]) << 24 | uint64_t(key[1]) << 16
        | uint64_t(key[2]) << 8 | uint64_t(key[3]);
  }

  common::Buffer prependPrefix(common::Buffer key,
                               kagome::blockchain::prefix::Prefix key_column) {
    return common::Buffer{}.putUint8(key_column).put(std::move(key));
  }

}  // namespace kagome::blockchain
