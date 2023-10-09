/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/protocols/state_protocol.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/impl/protocols/protocol_base_impl.hpp"
#include "network/state_protocol_observer.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::blockchain {
  class GenesisBlockHash;
}

namespace kagome::network {

  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::ProtocolName;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;

  class StateProtocolImpl final
      : public StateProtocol,
        public std::enable_shared_from_this<StateProtocolImpl>,
        NonCopyable,
        NonMovable {
   public:
    StateProtocolImpl(libp2p::Host &host,
                      const application::ChainSpec &chain_spec,
                      const blockchain::GenesisBlockHash &genesis_hash,
                      std::shared_ptr<StateProtocolObserver> state_observer);

    bool start() override;

    const std::string &protocolName() const override {
      return kStateProtocolName;
    }

    void onIncomingStream(std::shared_ptr<Stream> stream) override;
    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override;

    void request(const PeerId &peer_id,
                 StateRequest state_request,
                 std::function<void(outcome::result<StateResponse>)>
                     &&response_handler) override;

    void readRequest(std::shared_ptr<Stream> stream);

    void writeResponse(std::shared_ptr<Stream> stream,
                       StateResponse state_response);

    void writeRequest(std::shared_ptr<Stream> stream,
                      StateRequest state_request,
                      std::function<void(outcome::result<void>)> &&cb);

    void readResponse(
        std::shared_ptr<Stream> stream,
        std::function<void(outcome::result<StateResponse>)> &&response_handler);

   private:
    inline static const auto kStateProtocolName = "StateProtocol"s;
    ProtocolBaseImpl base_;
    std::shared_ptr<StateProtocolObserver> state_observer_;
  };

}  // namespace kagome::network
