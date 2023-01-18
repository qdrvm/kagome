/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/block_header_repository_impl.hpp"

#include <optional>

#include "blockchain/block_tree_error.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "scale/scale.hpp"

using kagome::blockchain::prefix::Prefix;
using kagome::common::Hash256;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;

namespace kagome::blockchain {

  BlockHeaderRepositoryImpl::BlockHeaderRepositoryImpl(
      std::shared_ptr<storage::SpacedStorage> storage,
      std::shared_ptr<crypto::Hasher> hasher)
      : map_{storage->getSpace(storage::Space::kDefault)},
        hasher_{std::move(hasher)} {
    BOOST_ASSERT(hasher_);
  }

  outcome::result<BlockNumber> BlockHeaderRepositoryImpl::getNumberByHash(
      const Hash256 &hash) const {
    OUTCOME_TRY(key, idToLookupKey(*map_, hash));
    if (!key.has_value()) return BlockTreeError::HEADER_NOT_FOUND;
    auto maybe_number = lookupKeyToNumber(key.value());

    return maybe_number;
  }

  outcome::result<common::Hash256> BlockHeaderRepositoryImpl::getHashByNumber(
      const primitives::BlockNumber &number) const {
    OUTCOME_TRY(header, getBlockHeader(number));
    OUTCOME_TRY(enc_header, scale::encode(header));
    return hasher_->blake2b_256(enc_header);
  }

  outcome::result<primitives::BlockHeader>
  BlockHeaderRepositoryImpl::getBlockHeader(const BlockId &id) const {
    OUTCOME_TRY(header_opt, getWithPrefix(*map_, Prefix::HEADER, id));
    if (header_opt.has_value()) {
      return scale::decode<primitives::BlockHeader>(header_opt.value());
    }
    return BlockTreeError::HEADER_NOT_FOUND;
  }

  outcome::result<BlockStatus> BlockHeaderRepositoryImpl::getBlockStatus(
      const primitives::BlockId &id) const {
    return getBlockHeader(id).has_value() ? BlockStatus::InChain
                                          : BlockStatus::Unknown;
  }

}  // namespace kagome::blockchain
