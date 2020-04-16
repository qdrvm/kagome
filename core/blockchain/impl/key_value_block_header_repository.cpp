/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "blockchain/impl/key_value_block_header_repository.hpp"

#include <string_view>

#include <boost/optional.hpp>

#include "blockchain/impl/storage_util.hpp"
#include "common/hexutil.hpp"
#include "scale/scale.hpp"

using kagome::blockchain::prefix::Prefix;
using kagome::common::Hash256;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;

namespace kagome::blockchain {

  KeyValueBlockHeaderRepository::KeyValueBlockHeaderRepository(
      std::shared_ptr<storage::BufferStorage> map,
      std::shared_ptr<crypto::Hasher> hasher)
      : map_{std::move(map)}, hasher_{std::move(hasher)} {
    BOOST_ASSERT(hasher_);
  }

  outcome::result<BlockNumber> KeyValueBlockHeaderRepository::getNumberByHash(
      const Hash256 &hash) const {
    OUTCOME_TRY(key, idToLookupKey(*map_, hash));

    auto maybe_number = lookupKeyToNumber(key);

    return maybe_number;
  }

  outcome::result<common::Hash256>
  KeyValueBlockHeaderRepository::getHashByNumber(
      const primitives::BlockNumber &number) const {
    OUTCOME_TRY(header, getBlockHeader(number));
    OUTCOME_TRY(enc_header, scale::encode(header));
    return hasher_->blake2b_256(enc_header);
  }

  outcome::result<primitives::BlockHeader>
  KeyValueBlockHeaderRepository::getBlockHeader(const BlockId &id) const {
    auto header_res = getWithPrefix(*map_, Prefix::HEADER, id);
    if (!header_res) {
      return (isNotFoundError(header_res.error())) ? Error::BLOCK_NOT_FOUND
                                                   : header_res.error();
    }

    return scale::decode<primitives::BlockHeader>(header_res.value());
  }

  outcome::result<BlockStatus> KeyValueBlockHeaderRepository::getBlockStatus(
      const primitives::BlockId &id) const {
    return getBlockHeader(id).has_value() ? BlockStatus::InChain
                                          : BlockStatus::Unknown;
  }

}  // namespace kagome::blockchain
