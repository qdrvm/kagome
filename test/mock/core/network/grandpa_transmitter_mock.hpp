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
    void vote(network::GrandpaVote &&msg) {
      vote_rv(msg);
    }
    MOCK_METHOD1(vote_rv, void(network::GrandpaVote &));

    void finalize(network::FullCommitMessage &&msg) {
      finalize_rv(msg);
    }
    MOCK_METHOD1(finalize_rv, void(network::FullCommitMessage &));

    void catchUpRequest(const libp2p::peer::PeerId &pi,
                        network::CatchUpRequest &&msg) {
      catchUpRequest_rv(pi, msg);
    }
    MOCK_METHOD2(catchUpRequest_rv,
                 void(const libp2p::peer::PeerId &, network::CatchUpRequest &));

    void catchUpResponse(const libp2p::peer::PeerId &pi,
                         network::CatchUpResponse &&msg) {
      catchUpResponse_rv(pi, msg);
    }
    MOCK_METHOD2(catchUpResponse_rv,
                 void(const libp2p::peer::PeerId &,
                      network::CatchUpResponse &));
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_GRANDPATRANSMITTERMOCK
