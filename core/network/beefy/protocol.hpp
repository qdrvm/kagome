/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/beefy/types.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "network/types/roles.hpp"

namespace kagome::blockchain {
  class GenesisBlockHash;
}  // namespace kagome::blockchain

namespace kagome::network {
  class Beefy;
  struct StreamEngine;

  class BeefyJustificationProtocol
      : public RequestResponseProtocol<primitives::BlockNumber,
                                       consensus::beefy::BeefyJustification,
                                       ScaleMessageReadWriter> {
    static constexpr auto kName = "BeefyJustification";

   public:
    BeefyJustificationProtocol(libp2p::Host &host,
                               const blockchain::GenesisBlockHash &genesis,
                               std::shared_ptr<Beefy> beefy);

    std::optional<outcome::result<ResponseType>> onRxRequest(
        RequestType block, std::shared_ptr<Stream>) override;

    void onTxRequest(const RequestType &) override {}

   private:
    std::shared_ptr<Beefy> beefy_;
  };

  class BeefyProtocol final
      : public ProtocolBase,
        public std::enable_shared_from_this<BeefyProtocol> {
    static constexpr auto kName = "Beefy";

   public:
    BeefyProtocol(libp2p::Host &host,
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
        std::shared_ptr<consensus::beefy::BeefyGossipMessage> message);

   private:
    ProtocolBaseImpl base_;
    Roles roles_;
    std::shared_ptr<Beefy> beefy_;
    std::shared_ptr<StreamEngine> stream_engine_;
  };
}  // namespace kagome::network
