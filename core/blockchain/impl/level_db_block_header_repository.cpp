/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "level_db_block_header_repository.hpp"

#include <string_view>

#include <boost/optional.hpp>
#include "blockchain/impl/level_db_util.hpp"
#include "common/hexutil.hpp"
#include "scale/scale.hpp"
#include "storage/leveldb/leveldb_error.hpp"

using kagome::blockchain::prefix::Prefix;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;

namespace kagome::blockchain {

  LevelDbBlockHeaderRepository::LevelDbBlockHeaderRepository(
      LevelDbBlockHeaderRepository::Db &db,
      std::shared_ptr<hash::Hasher> hasher)
      : db_{db}, hasher_{std::move(hasher)} {
    BOOST_ASSERT(hasher_);
  }

  outcome::result<BlockNumber> LevelDbBlockHeaderRepository::getNumberByHash(
      const Hash256 &hash) const {
    OUTCOME_TRY(key, idToLookupKey(hash));

    auto maybe_number = lookupKeyToNumber(key);
    if (maybe_number.has_value()) {
      return maybe_number.value();
    }
    return maybe_number.error();
  }

  outcome::result<common::Hash256>
  LevelDbBlockHeaderRepository::getHashByNumber(
      const primitives::BlockNumber &number) const {
    OUTCOME_TRY(header, getBlockHeader(number));
    OUTCOME_TRY(enc_header, scale::encode(header));
    return hasher_->blake2_256(enc_header);
  }

  outcome::result<primitives::BlockHeader>
  LevelDbBlockHeaderRepository::getBlockHeader(const BlockId &id) const {
    OUTCOME_TRY(key, idToLookupKey(id));
    auto header = db_.get(prependPrefix(key, Prefix::HEADER));
    if (!header) {
      return (header.error() == kagome::storage::LevelDBError::NOT_FOUND)
          ? Error::NOT_FOUND
          : header.error();
    }
    return scale::decode<primitives::BlockHeader>(header.value());
  }

  outcome::result<BlockStatus> LevelDbBlockHeaderRepository::getBlockStatus(
      const primitives::BlockId &id) const {
    return getBlockHeader(id).has_value() ? BlockStatus::InChain
                                          : BlockStatus::Unknown;
  }

  outcome::result<Buffer> LevelDbBlockHeaderRepository::idToLookupKey(
      const BlockId &id) const {
    auto key = visit_in_place(
        id,
        [this](const primitives::BlockNumber &n) {
          auto key =
              prependPrefix(numberToIndexKey(n), Prefix::ID_TO_LOOKUP_KEY);
          return db_.get(key);
        },
        [this](const common::Hash256 &hash) {
          return db_.get(prependPrefix(Buffer{hash}, Prefix::ID_TO_LOOKUP_KEY));
        });
    if(!key && key.error() == storage::LevelDBError::NOT_FOUND) {
      return Error::NOT_FOUND;
    }
    return key;
  }

}  // namespace kagome::blockchain
