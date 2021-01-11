/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_HPP
#define KAGOME_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_HPP

#include <libp2p/peer/peer_id.hpp>

#include "primitives/authority.hpp"
#include "primitives/block.hpp"
#include "primitives/block_data.hpp"
#include "primitives/block_id.hpp"

namespace kagome::consensus {

  /**
   * @brief Iterates over the list of accessible peers and tries to fetch
   * missing blocks from them
   */
  class BabeSynchronizer {
   public:
    using BlocksHandler =
        std::function<void(boost::optional<std::reference_wrapper<
                               const std::vector<primitives::BlockData>>>)>;

    virtual ~BabeSynchronizer() = default;

    /**
     * Request blocks between provided ones
     * @param from block id of the first requested block
     * @param to block hash of the last requested block
     * @param block_list_handler handles received blocks
     */
    virtual void request(const primitives::BlockId &from,
                         const primitives::BlockHash &to,
                         const libp2p::peer::PeerId &peer_id,
                         const BlocksHandler &block_list_handler) = 0;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_HPP
