/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PROTOCOLBASEMOCK
#define KAGOME_NETWORK_PROTOCOLBASEMOCK

#include "network/protocol_base.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class ProtocolBaseMock : public virtual ProtocolBase {
   public:
    MOCK_CONST_METHOD0(protocol, const Protocol &());
    MOCK_METHOD0(start, bool());
    MOCK_METHOD0(stop, bool());
    MOCK_METHOD1(onIncomingStream, void(std::shared_ptr<Stream>));
    MOCK_METHOD2(
        newOutgoingStream,
        void(const PeerInfo &,
             const std::function<void(outcome::result<std::shared_ptr<Stream>>)>
                 &));

    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&handler)
        override {
      const auto h = std::move(handler);
      newOutgoingStream(peer_info, h);
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PROTOCOLBASEMOCK
