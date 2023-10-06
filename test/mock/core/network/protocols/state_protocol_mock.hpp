/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_STATEPROTOCOLMOCK
#define KAGOME_NETWORK_STATEPROTOCOLMOCK

#include "mock/core/network/protocol_base_mock.hpp"
#include "network/protocols/state_protocol.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class StateProtocolMock : public StateProtocol, public ProtocolBaseMock {
   public:
    MOCK_METHOD(void,
                request,
                (const PeerId &,
                 StateRequest,
                 const std::function<void(outcome::result<StateResponse>)> &));

    void request(const PeerId &peer_id,
                 StateRequest state_request,
                 std::function<void(outcome::result<StateResponse>)>
                     &&response_handler) override {
      const auto h = std::move(response_handler);
      request(peer_id, std::move(state_request), h);
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_STATEPROTOCOLMOCK
