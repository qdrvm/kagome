/**round_number_,
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_GRANDPATRANSMITTERIMPL
#define KAGOME_NETWORK_GRANDPATRANSMITTERIMPL

#include "network/grandpa_transmitter.hpp"

namespace kagome::network {
  class Router;

  class GrandpaTransmitterImpl final : public GrandpaTransmitter {
   public:
    GrandpaTransmitterImpl(std::shared_ptr<Router> router);

    void sendVoteMessage(GrandpaVote &&message) override;
    void sendNeighborMessage(GrandpaNeighborMessage &&message) override;
    void sendCommitMessage(FullCommitMessage &&message) override;
    void sendCatchUpRequest(const libp2p::peer::PeerId &peer_id,
                            CatchUpRequest &&message) override;
    void sendCatchUpResponse(const libp2p::peer::PeerId &peer_id,
                             CatchUpResponse &&message) override;

   private:
    std::shared_ptr<Router> router_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_GRANDPATRANSMITTERIMPL
