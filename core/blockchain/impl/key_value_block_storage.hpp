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

#ifndef KAGOME_KEY_VALUE_BLOCK_STORAGE_HPP
#define KAGOME_KEY_VALUE_BLOCK_STORAGE_HPP

#include "blockchain/block_storage.hpp"

#include "blockchain/impl/common.hpp"
#include "common/logger.hpp"
#include "crypto/hasher.hpp"

namespace kagome::blockchain {

  class KeyValueBlockStorage : public BlockStorage {
    using GenesisHandler = std::function<void(const primitives::Block &)>;

   public:
    enum class Error {
      BLOCK_EXISTS = 1,
      BODY_DOES_NOT_EXIST,
      JUSTIFICATION_DOES_NOT_EXIST
    };

    ~KeyValueBlockStorage() override = default;

    /**
     * Initialise block storage with a genesis block which is created inside
     * from merkle trie root
     * @param storage underlying storage (must be empty)
     * @param hasher a hasher instance
     */
    static outcome::result<std::shared_ptr<KeyValueBlockStorage>>
    createWithGenesis(common::Buffer state_root,
                      const std::shared_ptr<storage::BufferStorage> &storage,
                      std::shared_ptr<crypto::Hasher> hasher,
                      const GenesisHandler &on_genesis_created);

    outcome::result<primitives::BlockHeader> getBlockHeader(
        const primitives::BlockId &id) const override;
    outcome::result<primitives::BlockBody> getBlockBody(
        const primitives::BlockId &id) const override;
    outcome::result<primitives::BlockData> getBlockData(
        const primitives::BlockId &id) const override;
    outcome::result<primitives::Justification> getJustification(
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
    common::Logger logger_;
  };
}  // namespace kagome::blockchain

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, KeyValueBlockStorage::Error);

#endif  // KAGOME_KEY_VALUE_BLOCK_STORAGE_HPP
