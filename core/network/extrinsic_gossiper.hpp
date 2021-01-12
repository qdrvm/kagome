/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_EXTRINSIC_GOSSIPER_HPP
#define KAGOME_CORE_NETWORK_EXTRINSIC_GOSSIPER_HPP

#include "network/types/propagate_transactions.hpp"

namespace kagome::network {
  /**
   * Sends messages, related to author api, over the Gossip protocol
   */
  struct ExtrinsicGossiper {
    virtual ~ExtrinsicGossiper() = default;

    /**
     * Broadcasts PropagatedTransactions message in network
     * @param txs to be sent
     */
    virtual void propagateTransactions(
        gsl::span<const primitives::Transaction> txs) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_EXTRINSIC_GOSSIPER_HPP
