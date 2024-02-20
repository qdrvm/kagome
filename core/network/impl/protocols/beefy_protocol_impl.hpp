/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/beefy/types.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "network/protocols/beefy_protocol.hpp"
#include "network/types/roles.hpp"

namespace kagome::blockchain {
  class GenesisBlockHash;
}

namespace kagome::network {
  class Beefy;
  struct StreamEngine;
}  // namespace kagome::network

namespace kagome::network {

  class BeefyProtocolImpl final
      : public ProtocolBase,
        public std::enable_shared_from_this<BeefyProtocolImpl>,
        public BeefyProtocol {
    static constexpr auto kName = "BeefyProtocol";

   public:
    BeefyProtocolImpl(libp2p::Host &host,
                      const blockchain::GenesisBlockHash &genesis,
                      Roles roles,
                      std::shared_ptr<Beefy> beefy,
                      std::shared_ptr<StreamEngine> stream_engine);

    bool start() override;
    const std::string &protocolName() const override;
    void onIncomingStream(std::shared_ptr<Stream> stream) override;
    void newOutgoingStream(
        const PeerInfo &peer,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override;

    void broadcast(
        std::shared_ptr<consensus::beefy::BeefyGossipMessage> message) override;

   private:
    ProtocolBaseImpl base_;
    Roles roles_;
    std::shared_ptr<Beefy> beefy_;
    std::shared_ptr<StreamEngine> stream_engine_;
  };

}  // namespace kagome::network
