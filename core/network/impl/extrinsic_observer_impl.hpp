/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/extrinsic_observer.hpp"

#include "log/logger.hpp"

namespace kagome::transaction_pool {
  class TransactionPool;
}

namespace kagome::network {

  class ExtrinsicObserverImpl : public ExtrinsicObserver {
   public:
    explicit ExtrinsicObserverImpl(
        std::shared_ptr<kagome::transaction_pool::TransactionPool> pool);

    outcome::result<common::Hash256> onTxMessage(
        const primitives::Extrinsic &extrinsic) override;

   private:
    std::shared_ptr<kagome::transaction_pool::TransactionPool> pool_;
    log::Logger logger_;
  };

}  // namespace kagome::network
