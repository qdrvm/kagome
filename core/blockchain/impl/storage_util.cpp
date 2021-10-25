/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/storage_util.hpp"

#include "blockchain/impl/common.hpp"
#include "storage/database_error.hpp"

using kagome::blockchain::prefix::Prefix;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain, KeyValueRepositoryError, e) {
  using E = kagome::blockchain::KeyValueRepositoryError;
  switch (e) {
    case E::INVALID_KEY:
      return "Invalid storage key";
  }
  return "Unknown error";
}

namespace kagome::blockchain {

  outcome::result<void> putWithPrefix(storage::BufferStorage &map,
                                      prefix::Prefix prefix,
                                      BlockNumber num,
                                      Hash256 block_hash,
                                      const common::Buffer &value) {
    auto block_lookup_key = numberAndHashToLookupKey(num, block_hash);
    auto value_lookup_key = prependPrefix(block_lookup_key, prefix);
    auto num_to_idx_key =
        prependPrefix(numberToIndexKey(num), Prefix::ID_TO_LOOKUP_KEY);
    auto hash_to_idx_key =
        prependPrefix(Buffer{block_hash}, Prefix::ID_TO_LOOKUP_KEY);
    OUTCOME_TRY(map.put(num_to_idx_key, block_lookup_key));
    OUTCOME_TRY(map.put(hash_to_idx_key, block_lookup_key));
    return map.put(value_lookup_key, value);
  }

  outcome::result<boost::optional<common::Buffer>> getWithPrefix(
      const storage::BufferStorage &map,
      prefix::Prefix prefix,
      const primitives::BlockId &block_id) {
    OUTCOME_TRY(key, idToLookupKey(map, block_id));
    return map.tryGet(prependPrefix(key, prefix));
  }

  common::Buffer numberToIndexKey(primitives::BlockNumber n) {
    // TODO(Harrm) Figure out why exactly it is this way in substrate
    BOOST_ASSERT((n & 0xffffffff00000000) == 0);

    return {uint8_t(n >> 24u),
            uint8_t((n >> 16u) & 0xffu),
            uint8_t((n >> 8u) & 0xffu),
            uint8_t(n & 0xffu)};
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
      return outcome::failure(KeyValueRepositoryError::INVALID_KEY);
    }
    return (uint64_t(key[0]) << 24u) | (uint64_t(key[1]) << 16u)
           | (uint64_t(key[2]) << 8u) | uint64_t(key[3]);
  }

  common::Buffer prependPrefix(const common::Buffer &key,
                               prefix::Prefix key_column) {
    return common::Buffer{}
        .reserve(key.size() + 1)
        .putUint8(key_column)
        .put(key);
  }

}  // namespace kagome::blockchain
