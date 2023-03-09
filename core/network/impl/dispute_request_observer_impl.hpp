/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_DISPUTEREQUESTOBSERVERIMPL
#define KAGOME_NETWORK_DISPUTEREQUESTOBSERVERIMPL

#include "network/dispute_request_observer.hpp"

namespace kagome::network {

  class DisputeRequestObserverImpl final
      : public DisputeRequestObserver,
        public std::enable_shared_from_this<DisputeRequestObserverImpl> {
   public:
    outcome::result<void> onDisputeRequest(
        const DisputeRequest &request) override;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_DISPUTEREQUESTOBSERVERIMPL
