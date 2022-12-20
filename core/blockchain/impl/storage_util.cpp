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

  outcome::result<void> putNumberToIndexKey(
      storage::BufferStorage &map, const primitives::BlockInfo &block) {
    auto num_to_idx_key =
        prependPrefix(numberToIndexKey(block.number), Prefix::ID_TO_LOOKUP_KEY);
    auto block_lookup_key = numberAndHashToLookupKey(block.number, block.hash);
    return map.put(num_to_idx_key, std::move(block_lookup_key));
  }

  outcome::result<void> putWithPrefix(storage::BufferStorage &map,
                                      prefix::Prefix prefix,
                                      BlockNumber num,
                                      Hash256 block_hash,
                                      common::BufferOrView &&value) {
    auto block_lookup_key = numberAndHashToLookupKey(num, block_hash);

    auto hash_to_idx_key = prependPrefix(block_hash, Prefix::ID_TO_LOOKUP_KEY);
    auto value_lookup_key = prependPrefix(block_lookup_key, prefix);
    OUTCOME_TRY(map.put(hash_to_idx_key, std::move(block_lookup_key)));

    return map.put(value_lookup_key, std::move(value));
  }

  outcome::result<bool> hasWithPrefix(const storage::BufferStorage &map,
                                      prefix::Prefix prefix,
                                      const primitives::BlockId &block_id) {
    OUTCOME_TRY(key, idToLookupKey(map, block_id));
    if (!key.has_value()) return false;
    return map.contains(prependPrefix(key.value(), prefix));
  }

  outcome::result<std::optional<common::BufferOrView>> getWithPrefix(
      const storage::BufferStorage &map,
      prefix::Prefix prefix,
      const primitives::BlockId &block_id) {
    OUTCOME_TRY(key, idToLookupKey(map, block_id));
    if (!key.has_value()) return std::nullopt;
    return map.tryGet(prependPrefix(key.value(), prefix));
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
      const common::BufferView &key) {
    if (key.size() < 4) {
      return outcome::failure(KeyValueRepositoryError::INVALID_KEY);
    }
    return (uint64_t(key[0]) << 24u) | (uint64_t(key[1]) << 16u)
           | (uint64_t(key[2]) << 8u) | uint64_t(key[3]);
  }

  common::Buffer prependPrefix(common::BufferView key,
                               prefix::Prefix key_column) {
    return common::Buffer{}
        .reserve(key.size() + 1)
        .putUint8(key_column)
        .put(key);
  }

}  // namespace kagome::blockchain
