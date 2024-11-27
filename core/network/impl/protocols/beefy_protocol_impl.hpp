/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/notifications/protocol.hpp"
#include "network/protocols/beefy_protocol.hpp"
#include "network/types/roles.hpp"

namespace kagome::blockchain {
  class GenesisBlockHash;
}  // namespace kagome::blockchain

namespace kagome::network {
  class Beefy;
}  // namespace kagome::network

namespace kagome::network {
  using libp2p::PeerId;

  class BeefyProtocolImpl final
      : public std::enable_shared_from_this<BeefyProtocolImpl>,
        public notifications::Controller,
        public BeefyProtocol {
    static constexpr auto kName = "BeefyProtocol";

   public:
    BeefyProtocolImpl(const notifications::Factory &notifications_factory,
                      const blockchain::GenesisBlockHash &genesis,
                      Roles roles,
                      std::shared_ptr<Beefy> beefy);

    // Controller
    Buffer handshake() override;
    bool onHandshake(const PeerId &peer_id,
                     size_t,
                     bool,
                     Buffer &&handshake) override;
    bool onMessage(const PeerId &peer_id, size_t, Buffer &&message) override;
    void onClose(const PeerId &peer_id) override;

    // BeefyProtocol
    void broadcast(
        std::shared_ptr<consensus::beefy::BeefyGossipMessage> message) override;

    void start();

   private:
    std::shared_ptr<notifications::Protocol> notifications_;
    Roles roles_;
    std::shared_ptr<Beefy> beefy_;
  };
}  // namespace kagome::network
