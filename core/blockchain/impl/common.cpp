/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/common.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "common/visitor.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"

namespace kagome::blockchain {

  outcome::result<std::optional<common::BufferOrView>> idToLookupKey(
      const ReadableBufferStorage &map, const primitives::BlockId &id) {
    auto key = visit_in_place(
        id,
        [](const primitives::BlockNumber &n) {
          return prependPrefix(numberToIndexKey(n),
                               prefix::Prefix::ID_TO_LOOKUP_KEY);
        },
        [](const common::Hash256 &hash) {
          return prependPrefix(hash, prefix::Prefix::ID_TO_LOOKUP_KEY);
        });

    OUTCOME_TRY(key_opt, map.tryGet(key));

    return std::move(key_opt);
  }
}  // namespace kagome::blockchain
