/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_SYNCPROTOCOLMOCK
#define KAGOME_NETWORK_SYNCPROTOCOLMOCK

#include "mock/core/network/protocol_base_mock.hpp"
#include "network/protocols/sync_protocol.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class SyncProtocolMock : public SyncProtocol, public ProtocolBaseMock {
   public:
    MOCK_METHOD(void,
                request,
                (const PeerId &,
                 BlocksRequest,
                 const std::function<void(outcome::result<BlocksResponse>)> &));

    void request(const PeerId &peer_id,
                 BlocksRequest block_request,
                 std::function<void(outcome::result<BlocksResponse>)>
                     &&response_handler) override {
      const auto h = std::move(response_handler);
      request(peer_id, std::move(block_request), h);
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_SYNCPROTOCOLMOCK
