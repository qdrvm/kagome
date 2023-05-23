/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_DISPUTEREQUESTOBSERVER
#define KAGOME_NETWORK_DISPUTEREQUESTOBSERVER

#include "outcome/outcome.hpp"

namespace kagome::network {
  struct DisputeMessage;
}

namespace kagome::network {

  class DisputeRequestObserver {
   public:
    using CbOutcome = std::function<void(outcome::result<void>)>;

    virtual ~DisputeRequestObserver() = default;

    virtual void onDisputeRequest(const DisputeMessage &request,
                                  CbOutcome &&cb) = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_DISPUTEREQUESTOBSERVER
