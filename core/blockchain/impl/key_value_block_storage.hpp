/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KEY_VALUE_BLOCK_STORAGE_HPP
#define KAGOME_KEY_VALUE_BLOCK_STORAGE_HPP

#include "blockchain/block_storage.hpp"

#include "blockchain/impl/common.hpp"
#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::blockchain {

  class KeyValueBlockStorage : public BlockStorage {
   public:
    using BlockHandler = std::function<void(const primitives::Block &)>;

    ~KeyValueBlockStorage() override = default;

    /**
     * Creates block storage. Iff block storage is empty, then initializes with
     * a genesis block which is created automatically from merkle trie root
     * @param state_root merkle root of genesis state
     * @param storage underlying storage (must be empty)
     * @param hasher a hasher instance
     */
    static outcome::result<std::shared_ptr<KeyValueBlockStorage>> create(
        storage::trie::RootHash state_root,
        const std::shared_ptr<storage::BufferStorage> &storage,
        const std::shared_ptr<crypto::Hasher> &hasher);

    outcome::result<std::vector<primitives::BlockHash>> getBlockTreeLeaves()
        const override;
    outcome::result<void> setBlockTreeLeaves(
        std::vector<primitives::BlockHash> leaves) override;

    outcome::result<bool> hasBlockHeader(
        const primitives::BlockId &id) const override;

    outcome::result<std::optional<primitives::BlockHeader>> getBlockHeader(
        const primitives::BlockId &id) const override;
    outcome::result<std::optional<primitives::BlockBody>> getBlockBody(
        const primitives::BlockId &id) const override;
    outcome::result<std::optional<primitives::BlockData>> getBlockData(
        const primitives::BlockId &id) const override;
    outcome::result<std::optional<primitives::Justification>> getJustification(
        const primitives::BlockId &block) const override;

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
        const primitives::BlockHash &hash,
        const primitives::BlockNumber &number) override;

   private:
    KeyValueBlockStorage(std::shared_ptr<storage::BufferStorage> storage,
                         std::shared_ptr<crypto::Hasher> hasher);

    std::shared_ptr<storage::BufferStorage> storage_;
    std::shared_ptr<crypto::Hasher> hasher_;
    log::Logger logger_;
    std::optional<primitives::BlockHash> genesis_block_hash_;

    mutable std::optional<std::vector<primitives::BlockHash>>
        block_tree_leaves_;
  };
}  // namespace kagome::blockchain

#endif  // KAGOME_KEY_VALUE_BLOCK_STORAGE_HPP
