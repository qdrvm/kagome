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

#include "blockchain/block_header_repository.hpp"

#include "common/visitor.hpp"

namespace kagome::blockchain {

  auto BlockHeaderRepository::getNumberById(const primitives::BlockId &id) const
      -> outcome::result<primitives::BlockNumber> {
    return visit_in_place(
        id, [](const primitives::BlockNumber &n) { return n; },
        [this](const common::Hash256 &hash) { return getNumberByHash(hash); });
  }

  /**
   * @param id of a block which hash is returned
   * @return block hash or a none optional if the corresponding block header
   * is not in storage or a storage error
   */
  auto BlockHeaderRepository::getHashById(const primitives::BlockId &id) const
      -> outcome::result<common::Hash256> {
    return visit_in_place(
        id,
        [this](const primitives::BlockNumber &n) { return getHashByNumber(n); },
        [](const common::Hash256 &hash) { return hash; });
  }

}  // namespace kagome::blockchain
