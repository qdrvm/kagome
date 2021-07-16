/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_GRANDPATRANSMITTERIMPL
#define KAGOME_NETWORK_GRANDPATRANSMITTERIMPL

#include "network/grandpa_transmitter.hpp"

#include "network/router.hpp"

namespace kagome::network {

  class GrandpaTransmitterImpl final : public GrandpaTransmitter {
   public:
    GrandpaTransmitterImpl(std::shared_ptr<network::Router> router);

    void vote(network::GrandpaVote &&message) override;
    void finalize(network::FullCommitMessage &&message) override;
    void catchUpRequest(const libp2p::peer::PeerId &peer_id,
                        network::CatchUpRequest &&message) override;
    void catchUpResponse(const libp2p::peer::PeerId &peer_id,
                         network::CatchUpResponse &&message) override;

   private:
    std::shared_ptr<network::Router> router_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_GRANDPATRANSMITTERIMPL
