/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_GRANDPATRANSMITTERMOCK
#define KAGOME_NETWORK_GRANDPATRANSMITTERMOCK

#include "network/grandpa_transmitter.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class GrandpaTransmitterMock : public GrandpaTransmitter {
   public:
    MOCK_METHOD(void, sendVoteMessage, (network::GrandpaVote &), ());
    void sendVoteMessage(network::GrandpaVote &&msg) override {
      sendVoteMessage(msg);
    }

    MOCK_METHOD(void,
                sendNeighborMessage,
                (network::GrandpaNeighborMessage &),
                ());
    void sendNeighborMessage(network::GrandpaNeighborMessage &&msg) override {
      sendNeighborMessage(msg);
    }

    MOCK_METHOD(void, sendCommitMessage, (network::FullCommitMessage &), ());
    void sendCommitMessage(network::FullCommitMessage &&msg) override {
      sendCommitMessage(msg);
    }

    MOCK_METHOD(void,
                sendCatchUpRequest,
                (const libp2p::peer::PeerId &, network::CatchUpRequest &),
                ());
    void sendCatchUpRequest(const libp2p::peer::PeerId &pi,
                            network::CatchUpRequest &&msg) override {
      sendCatchUpRequest(pi, msg);
    }

    MOCK_METHOD(void,
                sendCatchUpResponse,
                (const libp2p::peer::PeerId &, network::CatchUpResponse &),
                ());
    void sendCatchUpResponse(const libp2p::peer::PeerId &pi,
                             network::CatchUpResponse &&msg) override {
      sendCatchUpResponse(pi, msg);
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_GRANDPATRANSMITTERMOCK
