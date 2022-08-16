/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_STATEPROTOCOLIMPL
#define KAGOME_NETWORK_STATEPROTOCOLIMPL

#include "network/protocols/state_protocol.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/state_protocol_observer.hpp"

namespace kagome::network {

  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::Protocol;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;

  class StateProtocolImpl final
      : public StateProtocol,
        public std::enable_shared_from_this<StateProtocolImpl> {
   public:
    StateProtocolImpl(libp2p::Host &host,
                      const application::ChainSpec &chain_spec,
                      std::shared_ptr<StateProtocolObserver> state_observer);

    bool start() override;
    bool stop() override;

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
    const static inline auto kStateProtocolName = "StateProtocol"s;
    libp2p::Host &host_;
    std::shared_ptr<StateProtocolObserver> state_observer_;
    const libp2p::peer::Protocol protocol_;
    log::Logger log_ = log::createLogger("StateProtocol", "state_protocol");
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_STATEPROTOCOLIMPL
