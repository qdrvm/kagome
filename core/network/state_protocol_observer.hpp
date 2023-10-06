/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STATE_PROTOCOL_OBSERVER_HPP
#define KAGOME_STATE_PROTOCOL_OBSERVER_HPP

#include <outcome/outcome.hpp>
#include "network/types/state_request.hpp"
#include "network/types/state_response.hpp"

namespace kagome::network {
  /**
   * Reactive part of State protocol
   */
  class StateProtocolObserver {
   public:
    virtual ~StateProtocolObserver() = default;

    /**
     * Process a state request
     * @param request to be processed
     * @return state request or error
     */
    virtual outcome::result<StateResponse> onStateRequest(
        const StateRequest &request) const = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_STATE_PROTOCOL_OBSERVER_HPP
