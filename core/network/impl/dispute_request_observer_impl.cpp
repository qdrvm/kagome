/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_request_observer_impl.hpp"

#include "network/types/collator_messages.hpp"

namespace kagome::network {

  outcome::result<void> DisputeRequestObserverImpl::onDisputeRequest(
      const DisputeRequest &request) {
    // TODO(xDimon): Implementation needed
    throw std::runtime_error("Not implemented yet");
    return outcome::success();
  }

}  // namespace kagome::network
