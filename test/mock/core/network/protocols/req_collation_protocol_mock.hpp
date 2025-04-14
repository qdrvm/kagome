/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/protocols/req_collation_protocol.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class ReqCollationProtocolMock : public ReqCollationProtocol {
   public:
    std::vector<std::function<void(
        outcome::result<vstaging::CollationFetchingResponse>)>>
        cbs;

    void request(const PeerId &peer_id,
                 CollationFetchingRequest request,
                 std::function<void(outcome::result<CollationFetchingResponse>)>
                     &&response_handler) override {
      UNREACHABLE
    }

    void request(const PeerId &_,
                 vstaging::CollationFetchingRequest req,
                 std::function<
                     void(outcome::result<vstaging::CollationFetchingResponse>)>
                     &&response_handler) override {
      cbs.push_back(std::move(response_handler));
    }

    MOCK_METHOD(const ProtocolName &, protocolName, (), (const, override));

    MOCK_METHOD(bool, start, (), (override));

    MOCK_METHOD(void, onIncomingStream, (std::shared_ptr<Stream>), (override));

    MOCK_METHOD(
        void,
        newOutgoingStream,
        (const libp2p::peer::PeerId &,
         std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&),
        (override));
  };

}  // namespace kagome::network
