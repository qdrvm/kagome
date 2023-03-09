/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_DISPUTEREQUESTOBSERVER
#define KAGOME_NETWORK_DISPUTEREQUESTOBSERVER

#include "outcome/outcome.hpp"

namespace kagome::network {
  struct DisputeRequest;
}

namespace kagome::network {

  class DisputeRequestObserver {
   public:
    virtual ~DisputeRequestObserver() = default;

    virtual outcome::result<void> onDisputeRequest(
        const DisputeRequest &request) = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_DISPUTEREQUESTOBSERVER
