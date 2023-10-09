/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/types/block_announce.hpp"

#include "primitives/transaction.hpp"

namespace kagome::network {
  /**
   * Sends block announce
   */
  class TransactionsTransmitter {
   public:
    virtual ~TransactionsTransmitter() = default;

    /**
     * Send Transactions message
     * @param txs - list of transaction to be sent
     */
    virtual void propagateTransactions(
        gsl::span<const primitives::Transaction> txs) = 0;
  };
}  // namespace kagome::network
