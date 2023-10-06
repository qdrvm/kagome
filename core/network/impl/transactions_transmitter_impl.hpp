/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_TRANSACTIONSTRANSMITTERIMPL
#define KAGOME_NETWORK_TRANSACTIONSTRANSMITTERIMPL

#include "network/transactions_transmitter.hpp"

namespace kagome::network {
  class Router;

  class TransactionsTransmitterImpl final : public TransactionsTransmitter {
   public:
    TransactionsTransmitterImpl(std::shared_ptr<Router> router);

    void propagateTransactions(
        gsl::span<const primitives::Transaction> txs) override;

   private:
    std::shared_ptr<Router> router_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_TRANSACTIONSTRANSMITTERIMPL
