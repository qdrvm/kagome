/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_NETWORK_HPP
#define KAGOME_CONSENSUS_NETWORK_HPP

#include "primitives/block.hpp"

namespace kagome::consensus {
  /**
   * Convenience methods for consensus to interact with the network
   */
  struct ConsensusNetwork {
    virtual ~ConsensusNetwork() = default;

    /**
     * Broadcast a block to the known peers
     * @param block to be sent over the network
     */
    virtual void broadcast(const primitives::Block &block) = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_NETWORK_HPP
