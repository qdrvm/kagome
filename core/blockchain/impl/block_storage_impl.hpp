/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_BLOCKSTORAGEIMPL
#define KAGOME_BLOCKCHAIN_BLOCKSTORAGEIMPL

#include "blockchain/block_storage.hpp"

#include "blockchain/impl/common.hpp"
#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::blockchain {

  class BlockStorageImpl : public BlockStorage {
   public:
    ~BlockStorageImpl() override = default;

    /**
     * Creates block storage. Iff block storage is empty, then initializes with
     * a genesis block which is created automatically from merkle trie root
     * @param state_root merkle root of genesis state
     * @param storage underlying storage (must be empty)
     * @param hasher a hasher instance
     */
    static outcome::result<std::shared_ptr<BlockStorageImpl>> create(
        storage::trie::RootHash state_root,
        const std::shared_ptr<storage::BufferStorage> &storage,
        const std::shared_ptr<crypto::Hasher> &hasher);

    outcome::result<std::vector<primitives::BlockHash>> getBlockTreeLeaves()
        const override;
    outcome::result<void> setBlockTreeLeaves(
        std::vector<primitives::BlockHash> leaves) override;

    outcome::result<bool> hasBlockHeader(
        const primitives::BlockId &id) const override;

    outcome::result<primitives::BlockHeader> getBlockHeader(
        const primitives::BlockId &id) const override;
    outcome::result<primitives::BlockBody> getBlockBody(
        const primitives::BlockId &id) const override;
    outcome::result<primitives::BlockData> getBlockData(
        const primitives::BlockId &id) const override;
    outcome::result<primitives::Justification> getJustification(
        const primitives::BlockId &block) const override;

    outcome::result<void> putNumberToIndexKey(
        const primitives::BlockInfo &block) override;

    outcome::result<primitives::BlockHash> putBlockHeader(
        const primitives::BlockHeader &header) override;
    outcome::result<void> putBlockData(
        primitives::BlockNumber block_number,
        const primitives::BlockData &block_data) override;
    outcome::result<primitives::BlockHash> putBlock(
        const primitives::Block &block) override;

    outcome::result<void> putJustification(
        const primitives::Justification &j,
        const primitives::BlockHash &hash,
        const primitives::BlockNumber &number) override;

    outcome::result<void> removeBlock(
        const primitives::BlockInfo &block) override;

   private:
    BlockStorageImpl(std::shared_ptr<storage::BufferStorage> storage,
                     std::shared_ptr<crypto::Hasher> hasher);

    std::shared_ptr<storage::BufferStorage> storage_;
    std::shared_ptr<crypto::Hasher> hasher_;
    log::Logger logger_;

    mutable std::optional<std::vector<primitives::BlockHash>>
        block_tree_leaves_;
  };
}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCKCHAIN_BLOCKSTORAGEIMPL
