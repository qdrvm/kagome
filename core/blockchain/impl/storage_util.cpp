/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/storage_util.hpp"

#include "common/visitor.hpp"
#include "storage/database_error.hpp"

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;
using kagome::storage::Space;

namespace kagome::blockchain {

  outcome::result<std::optional<common::BufferOrView>> blockIdToBlockHash(
      storage::SpacedStorage &storage, const primitives::BlockId &block_id) {
    return visit_in_place(
        block_id,
        [&](const primitives::BlockNumber &block_number) {
          auto key_space = storage.getSpace(storage::Space::kLookupKey);
          return key_space->tryGet(blockNumberToKey(block_number));
        },
        [](const common::Hash256 &block_hash) {
          return std::make_optional(Buffer(block_hash));
        });
  }

  outcome::result<std::optional<primitives::BlockHash>> blockHashByNumber(
      storage::SpacedStorage &storage, primitives::BlockNumber block_number) {
    auto key_space = storage.getSpace(storage::Space::kLookupKey);
    OUTCOME_TRY(data_opt, key_space->tryGet(blockNumberToKey(block_number)));
    if (data_opt.has_value()) {
      OUTCOME_TRY(hash, primitives::BlockHash::fromSpan(data_opt.value()));
      return hash;
    }
    return std::nullopt;
  }

  outcome::result<bool> hasInSpace(storage::SpacedStorage &storage,
                                   storage::Space space,
                                   const primitives::BlockId &block_id) {
    OUTCOME_TRY(key, blockIdToBlockHash(storage, block_id));
    if (not key.has_value()) {
      return false;
    }

    auto target_space = storage.getSpace(space);
    return target_space->contains(key.value());
  }

  outcome::result<void> putToSpace(storage::SpacedStorage &storage,
                                   storage::Space space,
                                   const primitives::BlockHash &block_hash,
                                   common::BufferOrView &&value) {
    auto target_space = storage.getSpace(space);
    return target_space->put(block_hash, std::move(value));
  }

  outcome::result<std::optional<common::BufferOrView>> getFromSpace(
      storage::SpacedStorage &storage,
      storage::Space space,
      const primitives::BlockHash &block_hash) {
    auto target_space = storage.getSpace(space);
    return target_space->tryGet(block_hash);
  }

  outcome::result<void> removeFromSpace(
      storage::SpacedStorage &storage,
      storage::Space space,
      const primitives::BlockHash &block_hash) {
    auto target_space = storage.getSpace(space);
    return target_space->remove(block_hash);
  }

}  // namespace kagome::blockchain
