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
using kagome::common::Hash256;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;

namespace kagome::blockchain {

  LevelDbBlockHeaderRepository::LevelDbBlockHeaderRepository(
      PersistentBufferMap &db, std::shared_ptr<hash::Hasher> hasher)
      : db_{db}, hasher_{std::move(hasher)} {
    BOOST_ASSERT(hasher_);
  }

  outcome::result<BlockNumber> LevelDbBlockHeaderRepository::getNumberByHash(
      const Hash256 &hash) const {
    OUTCOME_TRY(key, idToLookupKey(db_, hash));

    auto maybe_number = lookupKeyToNumber(key);

    return maybe_number;
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
    auto header_res = getWithPrefix(db_, Prefix::HEADER, id);
    if (!header_res) {
      return (header_res.error() == kagome::storage::LevelDBError::NOT_FOUND)
          ? Error::BLOCK_NOT_FOUND
          : header_res.error();
    }
    return scale::decode<primitives::BlockHeader>(header_res.value());
  }

  outcome::result<BlockStatus> LevelDbBlockHeaderRepository::getBlockStatus(
      const primitives::BlockId &id) const {
    return getBlockHeader(id).has_value() ? BlockStatus::InChain
                                          : BlockStatus::Unknown;
  }

}  // namespace kagome::blockchain
