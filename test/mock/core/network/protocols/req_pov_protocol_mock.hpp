/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/protocols/req_pov_protocol.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class ReqPovProtocolMock : public IReqPovProtocol {
   public:
    std::unordered_map<
        RequestPov,
        std::function<void(outcome::result<ResponsePov>)>>
        cbs;

    void request(
        const libp2p::peer::PeerId &,
        RequestPov req,
        std::function<void(outcome::result<ResponsePov>)> &&cb)
        override {
      cbs[req] = std::move(cb);
    }

    MOCK_METHOD(const ProtocolName &,
                protocolName,
                (),
                (const, override));

    MOCK_METHOD(bool, start, (), (override));

    MOCK_METHOD(void,
                onIncomingStream,
                (std::shared_ptr<Stream>),
                (override));

    MOCK_METHOD(
        void,
        newOutgoingStream,
        (const libp2p::peer::PeerId &,
         std::function<
             void(outcome::result<std::shared_ptr<Stream>>)> &&),
        (override));
  };

}  // namespace kagome::network 