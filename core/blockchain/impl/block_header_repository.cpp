/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/block_header_repository.hpp"

#include "common/visitor.hpp"

namespace kagome::blockchain {

  auto BlockHeaderRepository::getNumberById(const primitives::BlockId &id) const
      -> outcome::result<primitives::BlockNumber> {
    return visit_in_place(
        id,
        [](const primitives::BlockNumber &n) { return n; },
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
