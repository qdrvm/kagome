/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/beefy/protocol.hpp"

#include "blockchain/genesis_block_hash.hpp"
#include "network/beefy/beefy.hpp"
#include "network/common.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/notifications/connect_and_handshake.hpp"
#include "network/notifications/handshake_and_read_messages.hpp"

namespace kagome::network {
  BeefyJustificationProtocol::BeefyJustificationProtocol(libp2p::Host &host,
                   const blockchain::GenesisBlockHash &genesis,
                   std::shared_ptr<Beefy> beefy)
          : RequestResponseProtocolType{
              kName,
              host,
              make_protocols(kBeefyJustificationProtocol, genesis),
              log::createLogger(kName),
          },
          beefy_{std::move(beefy)} {}

  std::optional<outcome::result<BeefyJustificationProtocol::ResponseType>>
  BeefyJustificationProtocol::onRxRequest(RequestType block,
                                          std::shared_ptr<Stream>) {
    OUTCOME_TRY(opt, beefy_->getJustification(block));
    if (opt) {
      return outcome::success(std::move(*opt));
    }
    return outcome::failure(ProtocolError::NO_RESPONSE);
  }

  BeefyProtocol::BeefyProtocol(libp2p::Host &host,
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

  bool BeefyProtocol::start() {
    return base_.start(weak_from_this());
  }

  const std::string &BeefyProtocol::protocolName() const {
    return base_.protocolName();
  }

  void BeefyProtocol::onIncomingStream(std::shared_ptr<Stream> stream) {
    auto on_handshake = [](std::shared_ptr<BeefyProtocol> self,
                           std::shared_ptr<Stream> stream,
                           Roles) {
      self->stream_engine_->addIncoming(stream, self);
      return true;
    };
    auto on_message = [](std::shared_ptr<BeefyProtocol> self,
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

  void BeefyProtocol::newOutgoingStream(
      const PeerInfo &peer,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto on_handshake =
        [cb = std::move(cb)](
            std::shared_ptr<BeefyProtocol> self,
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

  void BeefyProtocol::broadcast(
      std::shared_ptr<consensus::beefy::BeefyGossipMessage> message) {
    stream_engine_->broadcast(shared_from_this(), message);
  }
}  // namespace kagome::network
