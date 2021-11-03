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
    MOCK_METHOD(void, vote, (network::GrandpaVote &), ());
    void vote(network::GrandpaVote &&msg) override {
      vote(msg);
    }

    MOCK_METHOD(void, neighbor, (network::GrandpaNeighborMessage &), ());
    void neighbor(network::GrandpaNeighborMessage &&msg) override {
      neighbor(msg);
    }

    MOCK_METHOD(void, finalize, (network::FullCommitMessage &), ());
    void finalize(network::FullCommitMessage &&msg) override {
      finalize(msg);
    }

    MOCK_METHOD(void,
                catchUpRequest,
                (const libp2p::peer::PeerId &, network::CatchUpRequest &),
                ());
    void catchUpRequest(const libp2p::peer::PeerId &pi,
                        network::CatchUpRequest &&msg) override {
      catchUpRequest(pi, msg);
    }

    MOCK_METHOD(void,
                catchUpResponse,
                (const libp2p::peer::PeerId &, network::CatchUpResponse &),
                ());
    void catchUpResponse(const libp2p::peer::PeerId &pi,
                         network::CatchUpResponse &&msg) override {
      catchUpResponse(pi, msg);
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_GRANDPATRANSMITTERMOCK
