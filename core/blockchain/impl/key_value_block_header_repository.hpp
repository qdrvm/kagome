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

#ifndef KAGOME_CORE_BLOCKCHAIN_IMPL_KEY_VALUE_BLOCK_HEADER_REPOSITORY_HPP
#define KAGOME_CORE_BLOCKCHAIN_IMPL_KEY_VALUE_BLOCK_HEADER_REPOSITORY_HPP

#include "blockchain/block_header_repository.hpp"

#include "blockchain/impl/common.hpp"
#include "crypto/hasher.hpp"

namespace kagome::blockchain {

  class KeyValueBlockHeaderRepository : public BlockHeaderRepository {
   public:
    KeyValueBlockHeaderRepository(std::shared_ptr<storage::BufferStorage> map,
                                  std::shared_ptr<crypto::Hasher> hasher);

    ~KeyValueBlockHeaderRepository() override = default;

    auto getNumberByHash(const common::Hash256 &hash) const
        -> outcome::result<primitives::BlockNumber> override;

    auto getHashByNumber(const primitives::BlockNumber &number) const
        -> outcome::result<common::Hash256> override;

    auto getBlockHeader(const primitives::BlockId &id) const
        -> outcome::result<primitives::BlockHeader> override;

    auto getBlockStatus(const primitives::BlockId &id) const
        -> outcome::result<blockchain::BlockStatus> override;

   private:
    std::shared_ptr<storage::BufferStorage> map_;
    std::shared_ptr<crypto::Hasher> hasher_;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_CORE_BLOCKCHAIN_IMPL_KEY_VALUE_BLOCK_HEADER_REPOSITORY_HPP
