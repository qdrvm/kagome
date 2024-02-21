/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/beefy_protocol_impl.hpp"

#include "consensus/beefy/beefy.hpp"
#include "network/common.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/notifications/connect_and_handshake.hpp"
#include "network/notifications/handshake_and_read_messages.hpp"

namespace kagome::network {

  BeefyProtocolImpl::BeefyProtocolImpl(libp2p::Host &host,
                               const blockchain::GenesisBlockHash &genesis,
                               Roles roles,
                               std::shared_ptr<Beefy> beefy,
                               std::shared_ptr<StreamEngine>stream_engine
                               )
      : base_{
          kName,
          host,
          make_protocols(kBeefyProtocol, genesis),
          log::createLogger(kName),
        },
        roles_{roles},
        beefy_{std::move(beefy)},
        stream_engine_{std::move(stream_engine)}
        {}

  bool BeefyProtocolImpl::start() {
    return base_.start(weak_from_this());
  }

  const std::string &BeefyProtocolImpl::protocolName() const {
    return base_.protocolName();
  }

  void BeefyProtocolImpl::onIncomingStream(std::shared_ptr<Stream> stream) {
    auto on_handshake = [](std::shared_ptr<BeefyProtocolImpl> self,
                           std::shared_ptr<Stream> stream,
                           Roles) {
      self->stream_engine_->addIncoming(stream, self);
      return true;
    };
    auto on_message = [](std::shared_ptr<BeefyProtocolImpl> self,
                         consensus::beefy::BeefyGossipMessage message) {
      self->beefy_->onMessage(std::move(message));
      return true;
    };
    notifications::handshakeAndReadMessages<
        consensus::beefy::BeefyGossipMessage>(weak_from_this(),
                                              std::move(stream),
                                              roles_,
                                              std::move(on_handshake),
                                              std::move(on_message));
  }

  void BeefyProtocolImpl::newOutgoingStream(
      const PeerInfo &peer,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto on_handshake =
        [cb = std::move(cb)](
            std::shared_ptr<BeefyProtocolImpl> self,
            outcome::result<notifications::ConnectAndHandshake<Roles>>
                r) mutable {
          if (not r) {
            cb(r.error());
            return;
          }
          auto &stream = std::get<0>(r.value());
          self->stream_engine_->addOutgoing(stream, self);
          cb(std::move(stream));
        };
    notifications::connectAndHandshake(
        weak_from_this(), base_, peer, roles_, std::move(on_handshake));
  }

  void BeefyProtocolImpl::broadcast(
      std::shared_ptr<consensus::beefy::BeefyGossipMessage> message) {
    stream_engine_->broadcast(shared_from_this(), message);
  }
}  // namespace kagome::network
