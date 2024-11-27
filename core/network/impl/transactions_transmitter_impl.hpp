/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/transactions_transmitter.hpp"

namespace kagome::network {
  class Router;

  class TransactionsTransmitterImpl final : public TransactionsTransmitter {
   public:
    TransactionsTransmitterImpl(std::shared_ptr<Router> router);

    void propagateTransaction(primitives::Transaction tx) override;

   private:
    std::shared_ptr<Router> router_;
  };

}  // namespace kagome::network
