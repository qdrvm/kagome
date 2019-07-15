/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "level_db_block_header_repository.hpp"

#include <string_view>

#include "blockchain/impl/level_db_util.hpp"
#include "common/hexutil.hpp"
#include "scale/scale.hpp"

using kagome::blockchain::prefix::Prefix;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;

namespace kagome::blockchain {

  LevelDbBlockHeaderRepository::LevelDbBlockHeaderRepository(
      kagome::blockchain::LevelDbBlockHeaderRepository::Db& db,
      std::shared_ptr<kagome::hash::Hasher> hasher)
      : db_{db}, hasher_{std::move(hasher)} {}

  auto LevelDbBlockHeaderRepository::getNumberByHash(const Hash256 &hash) const
      -> outcome::result<boost::optional<BlockNumber>> {
    OUTCOME_TRY(key, idToLookupKey(hash));
    auto maybe_number = lookupKeyToNumber(std::move(key));
    if(maybe_number.has_value()) {
      return boost::make_optional(maybe_number.value());
    } else {
      return maybe_number.error();
    }
  }

  auto LevelDbBlockHeaderRepository::getHashByNumber(
      const primitives::BlockNumber &number) const
      -> outcome::result<boost::optional<common::Hash256>> {
    OUTCOME_TRY(header, getBlockHeader(number));
    OUTCOME_TRY(enc_header, scale::encode(std::move(header)));
    return hasher_->blake2_256(enc_header);
  }

  auto LevelDbBlockHeaderRepository::getBlockHeader(const BlockId &id) const
      -> outcome::result<primitives::BlockHeader> {
    OUTCOME_TRY(key, idToLookupKey(id));
    OUTCOME_TRY(header, db_.get(prependPrefix(key, Prefix::HEADER)));
    return scale::decode<primitives::BlockHeader>(std::move(header));
  }

  outcome::result<bool> LevelDbBlockHeaderRepository::getBlockStatus() const {}

  outcome::result<Buffer> LevelDbBlockHeaderRepository::idToLookupKey(
      const BlockId &id) const {
    auto key = visit_in_place(id,
                              [this](const primitives::BlockNumber &n) {
                                return db_.get(numberToIndexKey(n));
                              },
                              [this](const common::Hash256 &hash) {
                                return db_.get(prependPrefix(
                                    Buffer{hash}, Prefix::ID_TO_LOOKUP_KEY));
                              });
    return key;
  }

}  // namespace kagome::blockchain
