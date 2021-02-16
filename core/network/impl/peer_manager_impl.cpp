/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/peer_manager_impl.hpp"

#include <memory>

#include <libp2p/host/host.hpp>
#include <libp2p/multi/content_identifier_codec.hpp>
#include <libp2p/protocol/kademlia/impl/peer_routing_table.hpp>
#include <libp2p/protocol/kademlia/kademlia.hpp>

#include "network/common.hpp"
#include "outcome/outcome.hpp"

namespace kagome::network {
  PeerManagerImpl::PeerManagerImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      libp2p::Host &host,
      std::shared_ptr<libp2p::protocol::Identify> identify,
      std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia,
      std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
      std::shared_ptr<StreamEngine> stream_engine,
      const application::AppConfiguration &app_config,
      const application::ChainSpec &chain_spec,
      std::shared_ptr<clock::SteadyClock> clock,
      const BootstrapNodes &bootstrap_nodes,
      const OwnPeerInfo &own_peer_info,
      std::shared_ptr<network::SyncClientsSet> sync_clients)
      : app_state_manager_(std::move(app_state_manager)),
        host_(host),
        identify_(std::move(identify)),
        kademlia_(std::move(kademlia)),
        scheduler_(std::move(scheduler)),
        stream_engine_(std::move(stream_engine)),
        app_config_(app_config),
        chain_spec_(chain_spec),
        clock_(std::move(clock)),
        bootstrap_nodes_(bootstrap_nodes),
        own_peer_info_(own_peer_info),
        sync_clients_(std::move(sync_clients)),
        log_(common::createLogger("PeerManager")) {
    BOOST_ASSERT(app_state_manager_ != nullptr);
    BOOST_ASSERT(identify_ != nullptr);
    BOOST_ASSERT(kademlia_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    BOOST_ASSERT(stream_engine_ != nullptr);
    BOOST_ASSERT(sync_clients_ != nullptr);

    app_state_manager_->takeControl(*this);
  }

  bool PeerManagerImpl::prepare() {
    if (bootstrap_nodes_.empty()) {
      log_->critical(
          "Does not have any bootstrap nodes. "
          "Provide them by chain spec or CLI argument `--bootnodes'");
      return false;
    }

    return true;
  }

  bool PeerManagerImpl::start() {
    // Add themselves into peer routing
    kademlia_->addPeer(host_.getPeerInfo(), true);

    add_peer_handle_ =
        host_.getBus()
            .getChannel<libp2p::protocol::kademlia::events::PeerAddedChannel>()
            .subscribe([wp = weak_from_this()](const PeerId &peer_id) {
              if (auto self = wp.lock()) {
                self->processDiscoveredPeer(peer_id);
              }
            });

    identify_->onIdentifyReceived(
        [wp = weak_from_this()](const PeerId &peer_id) {
          if (auto self = wp.lock()) {
            self->processFullyConnectedPeer(peer_id);
          }
        });

    // Start Identify protocol
    identify_->start();

    // Enqueue bootstrap nodes as first peers set
    for (const auto &bootstrap_node : bootstrap_nodes_) {
      kademlia_->addPeer(bootstrap_node, true);
    }

    // Start Kademlia (processing incoming message and random walking)
    kademlia_->start();

    // Do first aligning of peers count
    align();

    return true;
  }

  void PeerManagerImpl::stop() {
    add_peer_handle_.unsubscribe();
  }

  void PeerManagerImpl::connectToPeer(const PeerInfo &peer_info) {
    auto res = host_.getPeerRepository().getAddressRepository().upsertAddresses(
        peer_info.id, peer_info.addresses, libp2p::peer::ttl::kTransient);
    if (res) {
      connectToPeer(peer_info.id);
    }
  }

  size_t PeerManagerImpl::activePeersNumber() const {
    return active_peers_.size();
  }

  void PeerManagerImpl::forEachPeer(
      std::function<void(const PeerId &)> func) const {
    for (auto &it : active_peers_) {
      func(it.first);
    }
  }

  void PeerManagerImpl::forOnePeer(
      const PeerId &peer_id, std::function<void(const PeerId &)> func) const {
    if (active_peers_.count(peer_id)) {
      func(peer_id);
    }
  }

  void PeerManagerImpl::align() {
    const auto target_count = app_config_.peeringConfig().targetPeerAmount;
    const auto soft_limit = app_config_.peeringConfig().softLimit;
    const auto hard_limit = app_config_.peeringConfig().hardLimit;
    const auto peer_ttl = app_config_.peeringConfig().peerTtl;

    align_timer_.cancel();

    // Check disconnected
    auto protocol =
        fmt::format(kBlockAnnouncesProtocol.data(), chain_spec_.protocolId());
    for (auto it = active_peers_.begin(); it != active_peers_.end();) {
      auto [peer_id, timepoint] = *it++;
      if (not stream_engine_->isAlive(peer_id, protocol)) {
        // Found disconnected
        log_->debug("Found dead peer_id={}", peer_id.toBase58());
        disconnectFromPeer(peer_id);
      }
    }

    // Soft limit is exceeded
    if (active_peers_.size() > soft_limit) {
      // Get oldest peer
      auto it = std::min_element(active_peers_.begin(),
                                 active_peers_.end(),
                                 [](const auto &item1, const auto &item2) {
                                   return item1.second < item2.second;
                                 });
      auto &[oldest_peer_id, oldest_timepoint] = *it;

      if (active_peers_.size() > hard_limit) {
        // Hard limit is exceeded
        log_->debug("Hard limit of of active peers is exceeded");
        disconnectFromPeer(oldest_peer_id);

      } else if (oldest_timepoint + peer_ttl < clock_->now()) {
        // Peer is inactive long time
        log_->debug("Found inactive peer_id={}", oldest_peer_id.toBase58());
        disconnectFromPeer(oldest_peer_id);

      } else {
        log_->debug("No peer to disconnect at soft limit");
      }
    }

    // Not enough active peers
    if (active_peers_.size() < target_count) {
      if (not queue_to_connect_.empty()) {
        auto node = peers_in_queue_.extract(queue_to_connect_.front());
        auto &peer_id = node.value();

        queue_to_connect_.pop_front();
        BOOST_ASSERT(queue_to_connect_.size() == peers_in_queue_.size());

        connecting_peers_.emplace(peer_id);
        connectToPeer(peer_id);

        log_->debug("Remained peers in queue for connect: {}",
                    peers_in_queue_.size());
      } else if (connecting_peers_.empty()) {
        log_->debug("Queue for connect is empty. Reuse bootstrap nodes");
        for (const auto &bootstrap_node : bootstrap_nodes_) {
          if (own_peer_info_.id != bootstrap_node.id) {
            connecting_peers_.emplace(bootstrap_node.id);
            connectToPeer(bootstrap_node.id);
          }
        }
      } else {
        log_->debug("Queue for connect is empty. Connecting peers: {}",
                    connecting_peers_.size());
      }
    }

    const auto aligning_period = app_config_.peeringConfig().aligningPeriod;

    align_timer_ = scheduler_->schedule(
        libp2p::protocol::scheduler::toTicks(aligning_period),
        [wp = weak_from_this()] {
          if (auto self = wp.lock()) {
            self->align();
          }
        });
  }

  void PeerManagerImpl::connectToPeer(const PeerId &peer_id) {
    auto peer_info = host_.getPeerRepository().getPeerInfo(peer_id);

    if (peer_info.addresses.empty()) {
      log_->debug("Not found addresses for peer_id={}", peer_id.toBase58());
      return;
    }

    auto connectedness =
        host_.getNetwork().getConnectionManager().connectedness(peer_info);
    if (connectedness
        == libp2p::network::ConnectionManager::Connectedness::CAN_NOT_CONNECT) {
      log_->debug("Can not connect to peer_id={}", peer_id.toBase58());
      return;
    }

    log_->debug("Try to connect to peer_id={}", peer_info.id.toBase58());
    for (auto addr : peer_info.addresses) {
      log_->debug("  address: {}", addr.getStringAddress());
    }

    host_.connect(
        peer_info, [wp = weak_from_this(), peer_id = peer_info.id](auto res) {
          auto self = wp.lock();
          if (not self) {
            return;
          }
          self->connecting_peers_.erase(peer_id);
          if (not res.has_value()) {
            self->log_->debug("Connecting to peer_id={} is failed: {}",
                              peer_id.toBase58(),
                              res.error().message());
            return;
          }
          auto &connection = res.value();
          auto remote_peer_id_res = connection->remotePeer();
          if (not remote_peer_id_res.has_value()) {
            self->log_->debug(
                "Connected, but not identifyed yet (expecting peer_id={})",
                peer_id.toBase58());
            return;
          }
          auto &remote_peer_id = remote_peer_id_res.value();
          if (remote_peer_id == peer_id) {
            self->log_->debug(
                "Perhaps has already connected to peer_id={}. "
                "Processing immediately",
                peer_id.toBase58());
            self->processFullyConnectedPeer(peer_id);
          }
        });
  }  // namespace kagome::network

  void PeerManagerImpl::disconnectFromPeer(const PeerId &peer_id) {
    auto it = active_peers_.find(peer_id);
    if (it != active_peers_.end()) {
      log_->debug("Disconnect from peer_id={}", peer_id.toBase58());
      stream_engine_->del(peer_id);
      active_peers_.erase(it);
      log_->debug("Remained {} active peers", active_peers_.size());
    }
    sync_clients_->remove(peer_id);
  }

  void PeerManagerImpl::keepAlive(const PeerId &peer_id) {
    auto it = active_peers_.find(peer_id);
    if (it != active_peers_.end()) {
      it->second = clock_->now();
    }
  }

  void PeerManagerImpl::processDiscoveredPeer(const PeerId &peer_id) {
    // Ignore himself
    if (own_peer_info_.id == peer_id) {
      return;
    }

    // Skip if peer is already active
    if (active_peers_.find(peer_id) != active_peers_.end()) {
      return;
    }

    auto [it, added] = peers_in_queue_.emplace(std::move(peer_id));

    // Already in queue
    if (not added) {
      return;
    }

    queue_to_connect_.emplace_back(*it);
    BOOST_ASSERT(queue_to_connect_.size() == peers_in_queue_.size());

    log_->debug("New peer_id={} enqueued. In queue: {}",
                (*it).toBase58(),
                queue_to_connect_.size());
  }

  void PeerManagerImpl::processFullyConnectedPeer(const PeerId &peer_id) {
    // Skip connection to himself
    if (own_peer_info_.id == peer_id) {
      return;
    }

    log_->debug("New connection with peer_id={}", peer_id.toBase58());

    auto addresses_res =
        host_.getPeerRepository().getAddressRepository().getAddresses(peer_id);

    if (not addresses_res.has_value()) {
      log_->debug("  addresses are not provided");
      return;
    }

    auto &addresses = addresses_res.value();
    for (auto addr : addresses) {
      log_->debug("  address: {}", addr.getStringAddress());
    }

    PeerInfo peer_info{.id = peer_id, .addresses = std::move(addresses)};

    const auto hard_limit = app_config_.peeringConfig().hardLimit;

    size_t cur_active_peer = active_peers_.size();

    // Capacity is allow
    if (cur_active_peer >= hard_limit) {
      connecting_peers_.erase(peer_id);

    } else {
      auto announce_protocol =
          fmt::format(kBlockAnnouncesProtocol.data(), chain_spec_.protocolId());

      if (not stream_engine_->isAlive(peer_info.id, announce_protocol)) {
        host_.newStream(
            peer_info,
            announce_protocol,
            [wp = weak_from_this(), peer_id = peer_info.id, announce_protocol](
                auto &&stream_res) {
              auto self = wp.lock();
              if (not self) return;

              // Remove from list of connecting peers
              self->connecting_peers_.erase(peer_id);

              if (!stream_res) {
                self->log_->warn("Unable to create '{}' stream with {}: {}",
                                 announce_protocol,
                                 peer_id.toBase58(),
                                 stream_res.error().message());
                self->disconnectFromPeer(peer_id);
                return;
              }

              // Add to active peer list
              if (auto [ap_it, ok] =
                      self->active_peers_.emplace(peer_id, self->clock_->now());
                  ok) {
                // And remove from queue
                if (auto piq_it = self->peers_in_queue_.find(peer_id);
                    piq_it != self->peers_in_queue_.end()) {
                  auto qtc_it =
                      std::find_if(self->queue_to_connect_.cbegin(),
                                   self->queue_to_connect_.cend(),
                                   [&peer_id = peer_id](const auto &item) {
                                     return peer_id == item.get();
                                   });
                  self->queue_to_connect_.erase(qtc_it);
                  self->peers_in_queue_.erase(piq_it);
                  BOOST_ASSERT(self->queue_to_connect_.size()
                               == self->peers_in_queue_.size());

                  self->log_->debug("Remained peers in queue for connect: {}",
                                    self->peers_in_queue_.size());
                }
              }

              self->log_->debug("Established {} stream with {}",
                                fmt::format(kBlockAnnouncesProtocol.data(),
                                            self->chain_spec_.protocolId()),
                                peer_id.toBase58());

              // Save initial payload stream (block-announce)
              [[maybe_unused]] auto res = self->stream_engine_->add(
                  stream_res.value(),
                  fmt::format(kBlockAnnouncesProtocol.data(),
                              self->chain_spec_.protocolId()));

              // Reserve stream slots for needed protocols

              self->stream_engine_->add(
                  peer_id,
                  fmt::format(kPropagateTransactionsProtocol.data(),
                              self->chain_spec_.protocolId()));

              self->stream_engine_->add(
                  peer_id,
                  fmt::format(kBlockAnnouncesProtocol.data(),
                              self->chain_spec_.protocolId()));

              self->stream_engine_->add(peer_id, kGossipProtocol);
            });
      }
    }

    kademlia_->addPeer(peer_info, false);
  }
}  // namespace kagome::network
