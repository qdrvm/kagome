/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP
#define KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP

#include <memory>

#include "common/logger.hpp"
#include "consensus/grandpa/grandpa.hpp"
#include "consensus/grandpa/grandpa_observer.hpp"
#include "libp2p/connection/stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"
#include "libp2p/protocol/identify.hpp"
#include "libp2p/protocol/ping.hpp"
#include "network/babe_observer.hpp"
#include "network/extrinsic_observer.hpp"
#include "network/gossiper.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/loopback_stream.hpp"
#include "network/router.hpp"
#include "network/sync_protocol_observer.hpp"
#include "network/types/bootstrap_nodes.hpp"
#include "network/types/gossip_message.hpp"
#include "network/types/own_peer_info.hpp"

namespace kagome::application {
  class ChainSpec;
}

namespace kagome::blockchain {
  class BlockStorage;
}

namespace kagome::network {
  class RouterLibp2p : public Router,
                       public std::enable_shared_from_this<RouterLibp2p> {
   public:
    RouterLibp2p(
        libp2p::Host &host,
        std::shared_ptr<BabeObserver> babe_observer,
        std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer,
        std::shared_ptr<SyncProtocolObserver> sync_observer,
        std::shared_ptr<ExtrinsicObserver> extrinsic_observer,
        std::shared_ptr<Gossiper> gossiper,
        const BootstrapNodes &bootstrap_nodes,
        const OwnPeerInfo &own_info,
        std::shared_ptr<kagome::application::ChainSpec> config,
        std::shared_ptr<blockchain::BlockStorage> storage,
        std::shared_ptr<libp2p::protocol::Identify> identify,
        std::shared_ptr<libp2p::protocol::Ping> ping_proto);

    ~RouterLibp2p() override = default;

    void init() override;

    void handleSyncProtocol(
        const std::shared_ptr<Stream> &stream) const override;

    void handleGossipProtocol(std::shared_ptr<Stream> stream) const override;

    void handleTransactionsProtocol(
        std::shared_ptr<Stream> stream) const override;

    void handleBlockAnnouncesProtocol(
        std::shared_ptr<Stream> stream) const override;

    void handleSupProtocol(std::shared_ptr<Stream> stream) const override;

   private:
    template <typename T, typename F>
    void readAsyncMsg(std::shared_ptr<Stream> stream, F &&f) const {
      auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);
      read_writer->read<T>([wp = weak_from_this(),
                            stream = std::move(stream),
                            f{std::forward<F>(f)}](auto &&msg_res) mutable {
        auto self = wp.lock();
        if (not self) return;

        if (not msg_res) {
          self->log_->error("error while reading message: {}",
                            msg_res.error().message());
          return stream->reset();
        }

        auto peer_id_res = stream->remotePeerId();
        if (not peer_id_res.has_value()) {
          self->log_->error("can't get peer_id: {}", msg_res.error().message());
          return stream->reset();
        }

        if (!std::forward<F>(f)(self, peer_id_res.value(), msg_res.value())) {
          stream->reset();
          return;
        }

        self->readAsyncMsg<T>(stream, std::forward<F>(f));
      });
    }

    template <typename T, typename H, typename F>
    void readAsyncMsgWithHandshake(std::shared_ptr<Stream> stream,
                                   H &&handshake,
                                   F &&f) const {
      auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);
      read_writer->read<std::decay_t<H>>([stream,
                                          handshake{std::move(handshake)},
                                          read_writer,
                                          wself{weak_from_this()},
                                          f{std::forward<F>(f)}](
                                             auto &&read_res) mutable {
        if (!read_res) {
          if (auto self = wself.lock())
            self->log_->error("Error while reading handshake: {}",
                              read_res.error().message());
          return stream->reset();
        }
        read_writer->write(
            handshake,
            [stream, wself, f{std::forward<F>(f)}](auto &&write_res) mutable {
              auto self = wself.lock();
              if (!self) return;

              if (!write_res) {
                self->log_->error("Error while reading handshake: {}",
                                  write_res.error().message());
                return stream->reset();
              }
              self->readAsyncMsg<std::decay_t<T>>(std::move(stream),
                                                  std::forward<F>(f));
            });
      });
    }

    /**
     * Process a received gossip message
     */
    bool processGossipMessage(const libp2p::peer::PeerId &peer_id,
                              const GossipMessage &msg) const;

    libp2p::Host &host_;
    std::shared_ptr<BabeObserver> babe_observer_;
    std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer_;
    std::shared_ptr<SyncProtocolObserver> sync_observer_;
    std::shared_ptr<ExtrinsicObserver> extrinsic_observer_;
    std::shared_ptr<Gossiper> gossiper_;
    std::weak_ptr<network::LoopbackStream> loopback_stream_;
    common::Logger log_;
    std::shared_ptr<kagome::application::ChainSpec> config_;
    libp2p::peer::Protocol transactions_protocol_;
    libp2p::peer::Protocol block_announces_protocol_;
    std::shared_ptr<blockchain::BlockStorage> storage_;
    std::shared_ptr<libp2p::protocol::Identify> identify_;
    std::shared_ptr<libp2p::protocol::Ping> ping_proto_;
    libp2p::event::Handle new_connection_handler_;
  };
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP
