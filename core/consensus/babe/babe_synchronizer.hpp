/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_HPP
#define KAGOME_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_HPP

#include "primitives/authority.hpp"
#include "primitives/block.hpp"
#include "primitives/block_id.hpp"

namespace kagome::consensus {

  /**
   * @brief Iterates over the list of accessible peers and tries to fetch
   * missing blocks from them
   */
  class BabeSynchronizer {
   public:
    using BlocksHandler =
        std::function<void(const std::vector<primitives::Block> &)>;

    virtual ~BabeSynchronizer() = default;

    /**
     * Request blocks between provided ones
     * @param from block id of the first requested block
     * @param to block hash of the last requested block
     * @param block_list_handler handles received blocks
     */
    virtual void request(const primitives::BlockId &from,
                         const primitives::BlockHash &to,
                         primitives::AuthorityIndex authority_index,
                         const BlocksHandler &block_list_handler) = 0;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_HPP
