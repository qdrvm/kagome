/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP
#define KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP

#include <memory>

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/grandpa/grandpa.hpp"
#include "consensus/grandpa/grandpa_observer.hpp"
#include "crypto/hasher.hpp"
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
#include "network/impl/stream_engine.hpp"
#include "network/peer_manager.hpp"
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
        std::shared_ptr<application::AppStateManager> app_state_manager,
        libp2p::Host &host,
        const application::AppConfiguration &app_config,
        std::shared_ptr<application::ChainSpec> chain_spec,
        const OwnPeerInfo &own_info,
        std::shared_ptr<StreamEngine> stream_engine,
        std::shared_ptr<BabeObserver> babe_observer,
        std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer,
        std::shared_ptr<SyncProtocolObserver> sync_observer,
        std::shared_ptr<ExtrinsicObserver> extrinsic_observer,
        std::shared_ptr<Gossiper> gossiper,
        const BootstrapNodes &bootstrap_nodes,
        std::shared_ptr<blockchain::BlockStorage> storage,
        std::shared_ptr<libp2p::protocol::Ping> ping_proto,
        std::shared_ptr<PeerManager> peer_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<crypto::Hasher> hasher);

    ~RouterLibp2p() override = default;

    /** @see AppStateManager::takeControl */
    bool prepare();

    /** @see AppStateManager::takeControl */
    bool start();

    /** @see AppStateManager::takeControl */
    void stop();

    void handleSyncProtocol(std::shared_ptr<Stream> stream) const override;

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

    template <typename T,
              typename Handshake,
              typename HandshakeHandler,
              typename MessageHandler>
    void readAsyncMsgWithHandshake(std::shared_ptr<Stream> stream,
                                   Handshake &&handshake,
                                   HandshakeHandler &&hh,
                                   MessageHandler &&mh) const {
      auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);
      read_writer->read<std::decay_t<Handshake>>(
          [stream,
           handshake{std::move(handshake)},
           read_writer,
           wself{weak_from_this()},
           hh{std::forward<HandshakeHandler>(hh)},
           mh{std::forward<MessageHandler>(mh)}](auto &&read_res) mutable {
            auto self = wself.lock();
            if (not self) {
              return stream->reset();
            }

            if (not read_res.has_value()) {
              self->log_->error("Error while reading handshake: {}",
                                read_res.error().message());
              return stream->reset();
            }

            auto res = hh(self,
                          stream->remotePeerId().value(),
                          std::move(read_res.value()));
            if (not res.has_value()) {
              self->log_->error("Error while processing handshake: {}",
                                read_res.error().message());
              return stream->reset();
            }

            read_writer->write(
                handshake,
                [stream, wself, mh{std::forward<MessageHandler>(mh)}](
                    auto &&write_res) mutable {
                  auto self = wself.lock();
                  if (not self) {
                    return;
                  }

                  if (not write_res.has_value()) {
                    self->log_->error("Error while writting handshake: {}",
                                      write_res.error().message());
                    return stream->reset();
                  }

                  self->readAsyncMsg<std::decay_t<T>>(
                      std::move(stream), std::forward<MessageHandler>(mh));
                });
          });
    }

    void setProtocolHandler(
        const libp2p::peer::Protocol &protocol,
        void (RouterLibp2p::*method)(std::shared_ptr<Stream>) const);

    /**
     * Process a received gossip message
     */
    bool processGossipMessage(const libp2p::peer::PeerId &peer_id,
                              const GossipMessage &msg) const;

    bool processSupMessage(const libp2p::peer::PeerId &peer_id,
                           const Status &msg) const;

    std::shared_ptr<application::AppStateManager> app_state_manager_;
    libp2p::Host &host_;
    const application::AppConfiguration &app_config_;
    std::shared_ptr<application::ChainSpec> chain_spec_;
    const OwnPeerInfo &own_info_;
    std::shared_ptr<StreamEngine> stream_engine_;
    std::shared_ptr<BabeObserver> babe_observer_;
    std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer_;
    std::shared_ptr<SyncProtocolObserver> sync_observer_;
    std::shared_ptr<ExtrinsicObserver> extrinsic_observer_;
    std::shared_ptr<Gossiper> gossiper_;
    std::weak_ptr<LoopbackStream> loopback_stream_;
    common::Logger log_;

    std::shared_ptr<blockchain::BlockStorage> storage_;
    std::shared_ptr<libp2p::protocol::Ping> ping_proto_;
    std::shared_ptr<PeerManager> peer_manager_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<crypto::Hasher> hasher_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP
