/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_IMPL_EXTRINSIC_OBSERVER_IMPL_HPP
#define KAGOME_CORE_NETWORK_IMPL_EXTRINSIC_OBSERVER_IMPL_HPP

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

#endif  // KAGOME_CORE_NETWORK_IMPL_EXTRINSIC_OBSERVER_IMPL_HPP
