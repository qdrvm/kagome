/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_BLOCKHEADERREPOSITORYIMPL
#define KAGOME_BLOCKCHAIN_BLOCKHEADERREPOSITORYIMPL

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
        const common::Hash256 &hash) const override;

    outcome::result<common::Hash256> getHashByNumber(
        const primitives::BlockNumber &number) const override;

    outcome::result<primitives::BlockHeader> getBlockHeader(
        const primitives::BlockId &id) const override;

    outcome::result<blockchain::BlockStatus> getBlockStatus(
        const primitives::BlockId &id) const override;

   private:
    std::shared_ptr<storage::SpacedStorage> storage_;
    std::shared_ptr<crypto::Hasher> hasher_;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCKCHAIN_BLOCKHEADERREPOSITORYIMPL
