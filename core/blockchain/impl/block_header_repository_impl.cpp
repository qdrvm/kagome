/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/block_header_repository_impl.hpp"

#include <optional>

#include "blockchain/block_tree_error.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "scale/scale.hpp"

using kagome::primitives::BlockHash;
using kagome::primitives::BlockNumber;
using kagome::storage::Space;

namespace kagome::blockchain {

  BlockHeaderRepositoryImpl::BlockHeaderRepositoryImpl(
      std::shared_ptr<storage::SpacedStorage> storage,
      std::shared_ptr<crypto::Hasher> hasher)
      : storage_{std::move(storage)}, hasher_{std::move(hasher)} {
    BOOST_ASSERT(hasher_);
  }

  outcome::result<BlockNumber> BlockHeaderRepositoryImpl::getNumberByHash(
      const primitives::BlockHash &hash) const {
    OUTCOME_TRY(header, getBlockHeader(hash));
    return header.number;
  }

  outcome::result<common::Hash256> BlockHeaderRepositoryImpl::getHashByNumber(
      primitives::BlockNumber number) const {
    auto num_to_idx_key = blockNumberToKey(number);
    auto key_space = storage_->getSpace(Space::kLookupKey);
    auto res = key_space->get(num_to_idx_key);
    if (not res.has_value()) {
      return BlockTreeError::HEADER_NOT_FOUND;
    }
    return common::Hash256::fromSpan(res.value());
  }

  outcome::result<primitives::BlockHeader>
  BlockHeaderRepositoryImpl::getBlockHeader(const primitives::BlockHash &block_hash) const {
          OUTCOME_TRY(header_opt,
                      getFromSpace(*storage_, Space::kHeader, block_hash));
          if (header_opt.has_value()) {
            return scale::decode<primitives::BlockHeader>(header_opt.value());
          }
          return BlockTreeError::HEADER_NOT_FOUND;
  }

  outcome::result<BlockStatus> BlockHeaderRepositoryImpl::getBlockStatus(
      const primitives::BlockHash &block_hash) const {
    return getBlockHeader(block_hash).has_value() ? BlockStatus::InChain
                                                  : BlockStatus::Unknown;
  }

}  // namespace kagome::blockchain
