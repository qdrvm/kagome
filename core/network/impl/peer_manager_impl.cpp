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
      const clock::SteadyClock &clock,
      const BootstrapNodes &bootstrap_nodes,
      const OwnPeerInfo &own_peer_info)
      : app_state_manager_(std::move(app_state_manager)),
        host_(host),
        identify_(std::move(identify)),
        kademlia_(std::move(kademlia)),
        scheduler_(std::move(scheduler)),
        stream_engine_(std::move(stream_engine)),
        app_config_(app_config),
        chain_spec_(chain_spec),
        clock_(clock),
        bootstrap_nodes_(bootstrap_nodes),
        own_peer_info_(own_peer_info),
        log_(common::createLogger("PeerManager")) {
    BOOST_ASSERT(app_state_manager_ != nullptr);
    BOOST_ASSERT(identify_ != nullptr);
    BOOST_ASSERT(kademlia_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    BOOST_ASSERT(stream_engine_ != nullptr);

    app_state_manager_->takeControl(*this);
  }

  bool PeerManagerImpl::prepare() {
    if (bootstrap_nodes_.empty()) {
      log_->critical("Does not have any bootstrap nodes");
      return false;
    }

    // Add bootstrap nodes into peer routing
    for (const auto &bootstrap_node : bootstrap_nodes_) {
      kademlia_->addPeer(bootstrap_node, true);
    }

    return true;
  }

  bool PeerManagerImpl::start() {
    // Add themselves into peer routing
    kademlia_->addPeer(host_.getPeerInfo(), true);

    add_peer_handle_ =
        host_.getBus()
            .getChannel<libp2p::protocol::kademlia::events::PeerAddedChannel>()
            .subscribe([wp = weak_from_this()](
                           const libp2p::peer::PeerId &peer_id) {
              auto self = wp.lock();
              if (not self) return;

              // This node
              if (self->own_peer_info_.id == peer_id) {
                return;
              }

              // Already active
              if (self->active_peers_.find(peer_id)
                  != self->active_peers_.end()) {
                return;
              }

              auto [it, added] =
                  self->peers_in_queue_.emplace(std::move(peer_id));

              // Already in queue
              if (not added) {
                return;
              }

              self->queue_to_connect_.emplace_back(*it);
              self->log_->debug("New peer_id={} enqueued", (*it).toBase58());
              self->align();
            });

    identify_->onIdentifyReceived([wp = weak_from_this()](auto &&peer_id) {
      auto self = wp.lock();
      if (not self) {
        return;
      }

      // Skip connection to himself
      if (self->own_peer_info_.id == peer_id) {
        return;
      }

      self->log_->debug("New connection with peer_id={}", peer_id.toBase58());

      auto addresses_res =
          self->host_.getPeerRepository().getAddressRepository().getAddresses(
              peer_id);

      if (not addresses_res.has_value()) {
        self->log_->debug("  addresses are not provided");
        return;
      }

      auto &addresses = addresses_res.value();
      for (auto addr : addresses) {
        self->log_->debug("  address: {}", addr.getStringAddress());
      }

      PeerInfo peer_info{.id = peer_id, .addresses = std::move(addresses)};
      self->kademlia_->addPeer(peer_info, false);

      // Add to active peer list
      if (auto [ap_it, ok] =
              self->active_peers_.emplace(peer_id, self->clock_.now());
          ok) {
        // And remove from queue
        if (auto piq_it = self->peers_in_queue_.find(peer_id);
            piq_it != self->peers_in_queue_.end()) {
          auto qtc_it = std::find_if(
              self->queue_to_connect_.cbegin(),
              self->queue_to_connect_.cend(),
              [&peer_id](const auto &item) { return peer_id == item.get(); });
          self->queue_to_connect_.erase(qtc_it);
        }

        self->host_.newStream(
            peer_info,
            fmt::format(kBlockAnnouncesProtocol.data(),
                        self->chain_spec_.protocolId()),
            [self, peer_info](auto &&stream_res) {
              if (!stream_res) {
                self->log_->warn("Unable to create stream with {}",
                                 peer_info.id.toHex());
                return;
              }
              [[maybe_unused]] auto res = self->stream_engine_->add(
                  stream_res.value(),
                  fmt::format(kBlockAnnouncesProtocol.data(),
                              self->chain_spec_.protocolId()));
            });

        // Reserve stream slots for needed protocols
        self->stream_engine_->add(
            peer_id,
            fmt::format(kPropagateTransactionsProtocol.data(),
                        self->chain_spec_.protocolId()));
        self->stream_engine_->add(peer_id,
                                  fmt::format(kBlockAnnouncesProtocol.data(),
                                              self->chain_spec_.protocolId()));
        self->stream_engine_->add(peer_id, kGossipProtocol);
      }
    });

    // Start Identify protocol
    identify_->start();

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

  void PeerManagerImpl::forEachPeer(
      std::function<void(const PeerId &)> func) const {
    std::for_each(active_peers_.begin(),
                  active_peers_.end(),
                  [&func](auto &item) { func(item.first); });
  }

  void PeerManagerImpl::forOnePeer(const PeerId &peer_id,
                                   std::function<void()> func) const {
    auto i = active_peers_.find(peer_id);
    if (i != active_peers_.end()) {
      func();
    }
  }

  void PeerManagerImpl::align() {
    const auto target_count = app_config_.peeringConfig().targetPeerAmount;
    const auto soft_limit = app_config_.peeringConfig().softLimit;
    const auto hard_limit = app_config_.peeringConfig().hardLimit;

    align_timer_.cancel();

    size_t cur_active_peer = active_peers_.size();

    // Not enough active peers
    if (cur_active_peer < target_count and not queue_to_connect_.empty()) {
      auto peer_id =
          std::move(peers_in_queue_.extract(queue_to_connect_.front()).value());
      queue_to_connect_.pop_front();

      connectToPeer(peer_id);
    }

    // Soft limit is exceeded
    if (cur_active_peer > soft_limit) {
      // Get oldest peer
      auto &&[oldest_peer_id, oldest_timepoint] = *active_peers_.begin();
      for (auto &[peer_id, timepoint] : active_peers_) {
        if (oldest_timepoint > timepoint) {
          const_cast<PeerId &>(oldest_peer_id) = peer_id;
        }
      }

      const auto peer_ttl = app_config_.peeringConfig().peerTtl;

      // Hard limit is exceeded OR peer is inactive long time
      if (cur_active_peer > hard_limit
          or oldest_timepoint + peer_ttl < clock_.now()) {
        // Disconnect from peer
        disconnectFromPeer(oldest_peer_id);
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
      return;
    }

    auto connectedness =
        host_.getNetwork().getConnectionManager().connectedness(peer_info);
    if (connectedness
        == libp2p::network::ConnectionManager::Connectedness::CAN_NOT_CONNECT) {
      return;
    }

    log_->debug("Try to connect to peer_id={}. In queue: {}",
                peer_info.id.toBase58(),
                queue_to_connect_.size());
    for (auto addr : peer_info.addresses) {
      log_->debug("  address: {}", addr.getStringAddress());
    }

    host_.connect(peer_info);
  }

  void PeerManagerImpl::disconnectFromPeer(const PeerId &peer_id) {
    auto it = active_peers_.find(peer_id);
    if (it != active_peers_.end()) {
      host_.disconnect(peer_id);
      active_peers_.erase(it);
    }
  }

  void PeerManagerImpl::keepAlive(const PeerId &peer_id) {
    auto it = active_peers_.find(peer_id);
    if (it != active_peers_.end()) {
      it->second = clock_.now();
    }
  }

}  // namespace kagome::network
