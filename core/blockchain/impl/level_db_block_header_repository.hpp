/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_BLOCKCHAIN_IMPL_LEVEL_DB_BLOCK_HEADER_REPOSITORY_HPP
#define KAGOME_CORE_BLOCKCHAIN_IMPL_LEVEL_DB_BLOCK_HEADER_REPOSITORY_HPP

#include "blockchain/block_header_repository.hpp"

#include "crypto/hasher.hpp"
#include "storage/face/persistent_map.hpp"

namespace kagome::blockchain {

  class LevelDbBlockHeaderRepository : public BlockHeaderRepository {
    using Db = storage::face::PersistentMap<common::Buffer, common::Buffer>;

   public:
    LevelDbBlockHeaderRepository(Db &db, std::shared_ptr<hash::Hasher> hasher);

    ~LevelDbBlockHeaderRepository() override = default;

    auto getNumberByHash(const common::Hash256 &hash) const
        -> outcome::result<primitives::BlockNumber> override;

    auto getHashByNumber(const primitives::BlockNumber &number) const
        -> outcome::result<common::Hash256> override;

    auto getBlockHeader(const primitives::BlockId &id) const
        -> outcome::result<primitives::BlockHeader> override;

    auto getBlockStatus(const primitives::BlockId &id) const
        -> outcome::result<blockchain::BlockStatus> override;

   private:
    outcome::result<common::Buffer> idToLookupKey(
        const primitives::BlockId &id) const;

    Db &db_;
    std::shared_ptr<hash::Hasher> hasher_;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_CORE_BLOCKCHAIN_IMPL_LEVEL_DB_BLOCK_HEADER_REPOSITORY_HPP
