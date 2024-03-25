/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"
#include "blockchain/genesis_block_hash.hpp"
#include "log/logger.hpp"
#include "network/collation_observer.hpp"
#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/protocol_base_impl.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/notifications/connect_and_handshake.hpp"
#include "network/notifications/handshake_and_read_messages.hpp"
#include "network/peer_manager.hpp"
#include "network/peer_view.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/collator_messages.hpp"
#include "network/types/roles.hpp"
#include "network/validation_observer.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  /**
   * @brief The `ParachainProtocol` class is a template class responsible for
   * handling the parachain protocol. It is a request-response protocol that is
   * used for both collation and validation protocols.
   * @tparam Observer - observer type
   * @tparam Message - message type
   * @tparam kCollation - A boolean value indicating whether the protocol is for
   * collation (true) or validation (false).
   */
  template <typename Observer,
            typename Message,
            bool kCollation,
            network::CollationVersion kProtoVersion>
  class ParachainProtocol
      : public ProtocolBase,
        public std::enable_shared_from_this<
            ParachainProtocol<Observer, Message, kCollation, kProtoVersion>>,
        NonCopyable,
        NonMovable {
   public:
    using ObserverType = Observer;
    using MessageType = Message;
    using Self =
        ParachainProtocol<Observer, Message, kCollation, kProtoVersion>;

    ParachainProtocol() = delete;
    ~ParachainProtocol() override = default;

    ParachainProtocol(libp2p::Host &host,
                      Roles roles,
                      const application::ChainSpec &chain_spec,
                      const blockchain::GenesisBlockHash &genesis_hash,
                      std::shared_ptr<ObserverType> observer,
                      const Protocol &protocol,
                      std::shared_ptr<network::PeerView> peer_view,
                      log::Logger logger)
        : base_(kParachainProtocolName,
                host,
                make_protocols(protocol, genesis_hash, kProtocolPrefixPolkadot),
                std::move(logger)),
          observer_(std::move(observer)),
          roles_{roles},
          protocol_{protocol},
          peer_view_(std::move(peer_view)) {
      BOOST_ASSERT(peer_view_);
    }

    /**
     * @brief Handles an incoming stream and starts the protocol
     *
     * @param stream The incoming stream.
     */
    void onIncomingStream(std::shared_ptr<Stream> stream) override {
      BOOST_ASSERT(stream->remotePeerId().has_value());

      // This lambda function is called when a handshake is received.
      auto on_handshake = [](std::shared_ptr<Self> self,
                             std::shared_ptr<Stream> stream,
                             Roles) {
        self->onHandshake(stream->remotePeerId().value());
        return true;
      };

      // This lambda function is called when a message is received.
      auto on_message = [peer_id = stream->remotePeerId().value()](
                            std::shared_ptr<Self> self,
                            WireMessage<MessageType> message) {
        visit_in_place(
            std::move(message),
            [&](ViewUpdate &&msg) {
              SL_TRACE(
                  self->base_.logger(), "Received ViewUpdate from {}", peer_id);
              self->peer_view_->updateRemoteView(peer_id, std::move(msg.view));
            },
            [&](MessageType &&p) {
              SL_TRACE(self->base_.logger(),
                       "Received Collation/Validation message from {}",
                       peer_id);
              self->observer_->onIncomingMessage(peer_id, std::move(p));
            });
        return true;
      };

      // Call the `handshakeAndReadMessages` function with the `on_handshake`
      // and `on_message` functions as arguments.
      notifications::handshakeAndReadMessages<WireMessage<MessageType>>(
          this->weak_from_this(),
          std::move(stream),
          roles_,
          std::move(on_handshake),
          std::move(on_message));
    }

    /**
     * @brief Initiates a new outgoing stream.
     *
     * @param peer_info The information of the peer to connect to.
     * @param cb The callback function to be called when the stream is
     * established.
     */
    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override {
      SL_DEBUG(base_.logger(),
               "Connect for {} stream with {}",
               protocolName(),
               peer_info.id);

      // This lambda function is called when a handshake is received.
      auto on_handshake =
          [cb = std::move(cb)](
              std::shared_ptr<Self> self,
              outcome::result<notifications::ConnectAndHandshake<Roles>>
                  r) mutable {
            if (not r) {
              cb(r.error());
              return;
            }
            auto &stream = std::get<0>(r.value());
            cb(std::move(stream));
          };
      notifications::connectAndHandshake(this->weak_from_this(),
                                         base_,
                                         peer_info,
                                         roles_,
                                         std::move(on_handshake));
    }

    /**
     * @brief Starts the protocol activating its logic in host's protocol
     * handler.
     *
     * @return true if the protocol is started successfully, false otherwise.
     */
    bool start() override {
      return base_.start(this->weak_from_this());
    }

    /**
     * @brief returns the protocol name which could be either collation or
     * validation protocol
     * @return the protocol name
     */
    const std::string &protocolName() const override {
      static std::optional<std::string> protocol_name;
      if (!protocol_name) {
        protocol_name = base_.protocolIds().size() > 0 ? base_.protocolIds()[0]
                                                       : base_.protocolName();
      }
      return protocol_name.value();
    }

   private:
    /**
     * @brief Handles the handshake with the peer.
     *
     * @param peer The peer to handshake with.
     */
    void onHandshake(const PeerId &peer) {
      if constexpr (kCollation) {
        observer_->onIncomingCollationStream(peer, kProtoVersion);
      } else {
        observer_->onIncomingValidationStream(peer, kProtoVersion);
      }
    }

    inline static const auto kParachainProtocolName = "ParachainProtocol"s;

    ProtocolBaseImpl base_;
    std::shared_ptr<ObserverType> observer_;
    Roles roles_;
    const Protocol protocol_;
    std::shared_ptr<network::PeerView> peer_view_;
  };

}  // namespace kagome::network
