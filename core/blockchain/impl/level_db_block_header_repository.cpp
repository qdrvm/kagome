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

using kagome::blockchain::prefix::Prefix;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;

namespace kagome::blockchain {

  LevelDbBlockHeaderRepository::LevelDbBlockHeaderRepository(
      kagome::blockchain::LevelDbBlockHeaderRepository::Db &db,
      std::shared_ptr<kagome::hash::Hasher> hasher)
      : db_{db}, hasher_{std::move(hasher)} {}

  outcome::result<BlockNumber> LevelDbBlockHeaderRepository::getNumberByHash(
      const Hash256 &hash) const {
    OUTCOME_TRY(key, idToLookupKey(hash));

    if (auto maybe_number = lookupKeyToNumber(key); maybe_number.has_value()) {
      return maybe_number.value();
    } else {
      return maybe_number.error();
    }
  }

  auto LevelDbBlockHeaderRepository::getHashByNumber(
      const primitives::BlockNumber &number) const
      -> outcome::result<common::Hash256> {
    OUTCOME_TRY(header, getBlockHeader(number));
    OUTCOME_TRY(enc_header, scale::encode(header));
    return hasher_->blake2_256(enc_header);
  }

  auto LevelDbBlockHeaderRepository::getBlockHeader(const BlockId &id) const
      -> outcome::result<primitives::BlockHeader> {
    OUTCOME_TRY(key, idToLookupKey(id));
    OUTCOME_TRY(header, db_.get(prependPrefix(key, Prefix::HEADER)));
    if (auto &&res = scale::decode<primitives::BlockHeader>(header); res) {
      return res.value();
    } else {
      return res.error();
    }
  }

  outcome::result<kagome::blockchain::BlockStatus, std::error_code,
                  boost::outcome_v2::policy::error_code_throw_as_system_error<
                      kagome::blockchain::BlockStatus, std::error_code, void>>
  LevelDbBlockHeaderRepository::getBlockStatus(
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
    return key;
  }

}  // namespace kagome::blockchain
