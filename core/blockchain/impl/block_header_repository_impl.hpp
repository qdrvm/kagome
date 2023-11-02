/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "blockchain/block_header_repository.hpp"

#include "crypto/hasher.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome::blockchain {

  class BlockHeaderRepositoryImpl : public BlockHeaderRepository {
   public:
    BlockHeaderRepositoryImpl(std::shared_ptr<storage::SpacedStorage> storage,
                              std::shared_ptr<crypto::Hasher> hasher);

    ~BlockHeaderRepositoryImpl() override = default;

    outcome::result<primitives::BlockNumber> getNumberByHash(
        const primitives::BlockHash &block_hash) const override;

    outcome::result<primitives::BlockHash> getHashByNumber(
        primitives::BlockNumber block_number) const override;

    outcome::result<primitives::BlockHeader> getBlockHeader(
        const primitives::BlockHash &block_hash) const override;

    outcome::result<blockchain::BlockStatus> getBlockStatus(
        const primitives::BlockHash &block_hash) const override;

   private:
    std::shared_ptr<storage::SpacedStorage> storage_;
    std::shared_ptr<crypto::Hasher> hasher_;
  };

}  // namespace kagome::blockchain
