/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/common.hpp"

#include "blockchain/impl/persistant_map_util.hpp"
#include "common/visitor.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain, Error, e) {
  switch (e) {
    case kagome::blockchain::Error::BLOCK_NOT_FOUND:
      return "Block with such ID is not found";
  }
  return "Unknown error";
}

namespace kagome::blockchain {
  using kagome::common::Buffer;

  outcome::result<common::Buffer> idToLookupKey(const ReadableBufferMap &map,
                                                const primitives::BlockId &id) {
    auto key = visit_in_place(
        id,
        [&map](const primitives::BlockNumber &n) {
          auto key = prependPrefix(numberToIndexKey(n),
                                   prefix::Prefix::ID_TO_LOOKUP_KEY);
          return map.get(key);
        },
        [&map](const common::Hash256 &hash) {
          return map.get(
              prependPrefix(Buffer{hash}, prefix::Prefix::ID_TO_LOOKUP_KEY));
        });
    if (!key && isNotFoundError(key.error())) {
      return Error::BLOCK_NOT_FOUND;
    }
    return key;
  }
}  // namespace kagome::blockchain
