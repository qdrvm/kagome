/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/storage_util.hpp"

#include "blockchain/impl/common.hpp"
#include "storage/database_error.hpp"

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;
using kagome::storage::Space;

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
      storage::SpacedStorage &storage, const primitives::BlockInfo &block) {
    auto num_to_idx_key = numberToIndexKey(block.number);
    auto block_lookup_key = numberAndHashToLookupKey(block.number, block.hash);
    auto key_space = storage.getSpace(Space::kLookupKey);
    return key_space->put(num_to_idx_key, std::move(block_lookup_key));
  }

  outcome::result<void> putToSpace(storage::SpacedStorage &storage,
                                   storage::Space space,
                                   primitives::BlockNumber num,
                                   common::Hash256 block_hash,
                                   common::BufferOrView &&value) {
    auto block_lookup_key = numberAndHashToLookupKey(num, block_hash);
    auto key_space = storage.getSpace(Space::kLookupKey);
    OUTCOME_TRY(key_space->put(block_hash, block_lookup_key.view()));
    OUTCOME_TRY(key_space->put(numberToIndexKey(num), block_lookup_key.view()));

    auto target_space = storage.getSpace(space);
    return target_space->put(block_lookup_key, std::move(value));
  }

  outcome::result<bool> hasInSpace(storage::SpacedStorage &storage,
                                   storage::Space space,
                                   const primitives::BlockId &block_id) {
    OUTCOME_TRY(key, idToLookupKey(storage, block_id));
    if (!key.has_value()) {
      return false;
    }
    auto target_space = storage.getSpace(space);
    return target_space->contains(key.value());
  }

  outcome::result<std::optional<common::BufferOrView>> getFromSpace(
      storage::SpacedStorage &storage,
      storage::Space space,
      const primitives::BlockId &block_id) {
    OUTCOME_TRY(key, idToLookupKey(storage, block_id));
    if (!key.has_value()) {
      return std::nullopt;
    }
    auto target_space = storage.getSpace(space);
    return target_space->tryGet(key.value());
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

}  // namespace kagome::blockchain
