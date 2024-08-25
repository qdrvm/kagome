/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/protocol_base.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class ProtocolBaseMock : public virtual ProtocolBase {
   public:
    MOCK_METHOD(const std::string &, protocolName, (), (const, override));

    MOCK_METHOD(bool, start, (), (override));

    MOCK_METHOD(void, onIncomingStream, (std::shared_ptr<Stream>), (override));

    MOCK_METHOD(
        void,
        newOutgoingStream,
        (const PeerId &,
         const std::function<void(outcome::result<std::shared_ptr<Stream>>)>
             &));

    void newOutgoingStream(
        const PeerId &peer_id,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&handler)
        override {
      const auto h = std::move(handler);
      newOutgoingStream(peer_id, h);
    }
  };

}  // namespace kagome::network
