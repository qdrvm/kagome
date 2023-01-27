/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/common.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "common/visitor.hpp"
#include "storage/in_memory/in_memory_storage.hpp"

namespace kagome::blockchain {

  outcome::result<std::optional<common::BufferOrView>> idToLookupKey(
      storage::SpacedStorage &storage, const primitives::BlockId &id) {
    auto key = visit_in_place(
        id,
        [](const primitives::BlockNumber &n) { return numberToIndexKey(n); },
        [](const common::Hash256 &hash) { return hash; });

    auto key_space = storage.getSpace(storage::Space::kLookupKey);
    OUTCOME_TRY(key_opt, key_space->tryGet(key));

    return std::move(key_opt);
  }
}  // namespace kagome::blockchain
