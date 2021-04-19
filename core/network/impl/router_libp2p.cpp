/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/router_libp2p.hpp"

namespace kagome::network {
  RouterLibp2p::RouterLibp2p(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      libp2p::Host &host,
      const application::AppConfiguration &app_config,
      const OwnPeerInfo &own_info,
      const BootstrapNodes &bootstrap_nodes,
      std::shared_ptr<libp2p::protocol::Ping> ping_proto,
      std::shared_ptr<network::ProtocolFactory> protocol_factory)
      : app_state_manager_{app_state_manager},
        host_{host},
        app_config_(app_config),
        own_info_{own_info},
        log_{log::createLogger("RouterLibp2p", "network")},
        ping_proto_{std::move(ping_proto)},
        protocol_factory_{std::move(protocol_factory)} {
    BOOST_ASSERT(app_state_manager_ != nullptr);
    BOOST_ASSERT(ping_proto_ != nullptr);
    BOOST_ASSERT(protocol_factory_ != nullptr);

    SL_DEBUG(log_, "Own peer id: {}", own_info.id.toBase58());
    if (!bootstrap_nodes.empty()) {
      for (const auto &peer_info : bootstrap_nodes) {
        for (auto &addr : peer_info.addresses) {
          SL_DEBUG(log_, "Bootstrap node: {}", addr.getStringAddress());
        }
      }
    } else if (app_config_.isRunInDevMode()) {
      SL_DEBUG(log_, "No bootstrap node. Dev mode.");
    } else {
      log_->error("No bootstrap node");
    }

    app_state_manager_->takeControl(*this);
  }

  bool RouterLibp2p::prepare() {
    host_.setProtocolHandler(
        ping_proto_->getProtocolId(), [wp = weak_from_this()](auto &&stream) {
          if (auto self = wp.lock()) {
            if (auto peer_id = stream->remotePeerId()) {
              self->log_->info("Handled {} protocol stream from: {}",
                               self->ping_proto_->getProtocolId(),
                               peer_id.value().toBase58());
              self->ping_proto_->handle(std::forward<decltype(stream)>(stream));
            }
          }
        });

    block_announce_protocol_ = protocol_factory_->makeBlockAnnounceProtocol();
    if (not block_announce_protocol_) {
      return false;
    }

    gossip_protocol_ = protocol_factory_->makeGossipProtocol();
    if (not gossip_protocol_) {
      return false;
    }

    grandpa_protocol_ = protocol_factory_->makeGrandpaProtocol();
    if (not grandpa_protocol_) {
      return false;
    }

    propagate_transaction_protocol_ =
        protocol_factory_->makePropagateTransactionsProtocol();
    if (not propagate_transaction_protocol_) {
      return false;
    }

    sup_protocol_ = protocol_factory_->makeSupProtocol();
    if (not sup_protocol_) {
      return false;
    }

    sync_protocol_ = protocol_factory_->makeSyncProtocol();
    if (not sync_protocol_) {
      return false;
    }

    block_announce_protocol_->start();
    gossip_protocol_->start();
    grandpa_protocol_->start();
    propagate_transaction_protocol_->start();
    sup_protocol_->start();
    sync_protocol_->start();

    return true;
  }

  bool RouterLibp2p::start() {
    // IPv6
    {
      auto ma_res = libp2p::multi::Multiaddress::create(
          "/ip6/::/tcp/" + std::to_string(app_config_.p2pPort()) + "/p2p/"
          + own_info_.id.toBase58());
      BOOST_ASSERT(ma_res.has_value());
      auto &ma = ma_res.value();
      auto res = host_.listen(ma);
      if (not res) {
        log_->error("Cannot listen address {}. Error: {}",
                    ma.getStringAddress(),
                    res.error().message());
      }
    }

    // IPv4
    {
      auto ma_res = libp2p::multi::Multiaddress::create(
          "/ip4/0.0.0.0/tcp/" + std::to_string(app_config_.p2pPort()) + "/p2p/"
          + own_info_.id.toBase58());
      BOOST_ASSERT(ma_res.has_value());
      auto &ma = ma_res.value();
      auto res = host_.listen(ma);
      if (not res) {
        log_->error("Cannot listen address {}. Error: {}",
                    ma.getStringAddress(),
                    res.error().message());
      }
    }

    auto &addr_repo = host_.getPeerRepository().getAddressRepository();
    auto upsert_res = addr_repo.upsertAddresses(
        own_info_.id, own_info_.addresses, libp2p::peer::ttl::kPermanent);
    if (!upsert_res) {
      log_->error("Cannot add own addresses to repo: {}",
                  upsert_res.error().message());
    }

    host_.start();

    const auto &host_addresses = host_.getAddresses();
    if (host_addresses.empty()) {
      log_->critical("Host addresses is empty");
      return false;
    }

    log_->info("Started with peer id: {}", host_.getId().toBase58());
    for (auto &addr : host_addresses) {
      log_->info("Started listening on address: {}", addr.getStringAddress());
    }

    return true;
  }

  void RouterLibp2p::stop() {
    if (host_.getNetwork().getListener().isStarted()) {
      host_.stop();
    }
  }

  std::shared_ptr<BlockAnnounceProtocol>
  RouterLibp2p::getBlockAnnounceProtocol() const {
    return block_announce_protocol_;
  }
  std::shared_ptr<GossipProtocol> RouterLibp2p::getGossipProtocol() const {
    return gossip_protocol_;
  }
  std::shared_ptr<PropagateTransactionsProtocol>
  RouterLibp2p::getPropagateTransactionsProtocol() const {
    return propagate_transaction_protocol_;
  }
  std::shared_ptr<SupProtocol> RouterLibp2p::getSupProtocol() const {
    return sup_protocol_;
  }
  std::shared_ptr<SyncProtocol> RouterLibp2p::getSyncProtocol() const {
    return sync_protocol_;
  }

}  // namespace kagome::network
