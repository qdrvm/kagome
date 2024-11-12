/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/peer_manager_impl.hpp"

#include <execinfo.h>
#include <limits>
#include <memory>

#include <libp2p/protocol/kademlia/impl/peer_routing_table.hpp>
#include <libp2p/protocol/ping.hpp>

#include "common/main_thread_pool.hpp"
#include "network/can_disconnect.hpp"
#include "scale/libp2p_types.hpp"
#include "storage/predefined_keys.hpp"
#include "utils/pool_handler_ready_make.hpp"

namespace {
  /// Reputation value for a node when we get disconnected from it.
  static constexpr int32_t kDisconnectReputation = -256;
  /// Reputation change for a node when we get disconnected from it.
  static constexpr int32_t kMinReputationForInnerConnection = -128;
  static constexpr int32_t kMinReputationForOuterConnection = -128;
}  // namespace

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network, PeerManagerImpl::Error, e) {
  using E = kagome::network::PeerManagerImpl::Error;
  switch (e) {
    case E::UNDECLARED_COLLATOR:
      return "Process handling from undeclared collator";
  }
  return "Unknown error in ChainSpecImpl";
}

namespace kagome::network {
  PeerManagerImpl::PeerManagerImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      libp2p::Host &host,
      common::MainThreadPool &main_thread_pool,
      std::shared_ptr<libp2p::protocol::Identify> identify,
      std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler,
      const application::AppConfiguration &app_config,
      std::shared_ptr<clock::SteadyClock> clock,
      const BootstrapNodes &bootstrap_nodes,
      const OwnPeerInfo &own_peer_info,
      std::shared_ptr<network::Router> router,
      std::shared_ptr<storage::SpacedStorage> storage,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<ReputationRepository> reputation_repository,
      LazySPtr<CanDisconnect> can_disconnect,
      std::shared_ptr<PeerView> peer_view)
      : log_{log::createLogger("PeerManager", "network")},
        host_(host),
        main_pool_handler_{poolHandlerReadyMake(
            this, app_state_manager, main_thread_pool, log_)},
        identify_(std::move(identify)),
        kademlia_(std::move(kademlia)),
        scheduler_(std::move(scheduler)),
        app_config_(app_config),
        clock_(std::move(clock)),
        bootstrap_nodes_(bootstrap_nodes),
        own_peer_info_(own_peer_info),
        router_{std::move(router)},
        storage_{storage->getSpace(storage::Space::kDefault)},
        hasher_{std::move(hasher)},
        reputation_repository_{std::move(reputation_repository)},
        can_disconnect_{can_disconnect},
        peer_view_{std::move(peer_view)} {
    BOOST_ASSERT(identify_ != nullptr);
    BOOST_ASSERT(kademlia_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    BOOST_ASSERT(router_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(peer_view_);
    BOOST_ASSERT(reputation_repository_ != nullptr);
    BOOST_ASSERT(peer_view_ != nullptr);

    app_state_manager->takeControl(*this);
  }

  bool PeerManagerImpl::tryStart() {
    if (not app_config_.isRunInDevMode() && bootstrap_nodes_.empty()) {
      log_->critical(
          "Does not have any bootstrap nodes. "
          "Provide them by chain spec or CLI argument `--bootnodes'");
    }

    if (app_config_.isRunInDevMode() && bootstrap_nodes_.empty()) {
      log_->warn(
          "Peer manager is started in passive mode, "
          "because have not any bootstrap nodes.");
      return true;
    }

    // Add themselves into peer routing
    kademlia_->addPeer(host_.getPeerInfo(), true);

    add_peer_handle_ =
        host_.getBus()
            .getChannel<libp2p::event::protocol::kademlia::PeerAddedChannel>()
            .subscribe([wp{weak_from_this()}](const PeerId &peer_id) {
              if (auto self = wp.lock()) {
                if (auto rating =
                        self->reputation_repository_->reputation(peer_id);
                    rating < 0) {
                  SL_DEBUG(self->log_,
                           "Disconnecting from peer {} due to its negative "
                           "reputation {}",
                           peer_id,
                           rating);
                  self->disconnectFromPeer(peer_id);
                  return;
                }
                self->processDiscoveredPeer(peer_id);
              }
            });

    peer_disconnected_handler_ =
        host_.getBus()
            .getChannel<libp2p::event::network::OnPeerDisconnectedChannel>()
            .subscribe([wp{weak_from_this()}](const PeerId &peer_id) {
              if (auto self = wp.lock()) {
                SL_DEBUG(self->log_,
                         "OnPeerDisconnectedChannel handler from peer {}",
                         peer_id);
                self->peer_states_.erase(peer_id);
                self->active_peers_.erase(peer_id);
                self->connecting_peers_.erase(peer_id);
                self->peer_view_->removePeer(peer_id);
                SL_DEBUG(self->log_,
                         "Remained {} active peers",
                         self->active_peers_.size());
              }
            });

    identify_->onIdentifyReceived([wp{weak_from_this()}](
                                      const PeerId &peer_id) {
      if (auto self = wp.lock()) {
        SL_DEBUG(self->log_, "Identify received from peer {}", peer_id);
        if (auto rating = self->reputation_repository_->reputation(peer_id);
            rating < 0) {
          SL_DEBUG(
              self->log_,
              "Disconnecting from peer {} due to its negative reputation {}",
              peer_id,
              rating);
          self->disconnectFromPeer(peer_id);
          return;
        }
        self->processFullyConnectedPeer(peer_id);
      }
    });

    // Start Identify protocol
    identify_->start();

    // Enqueue bootstrap nodes with permanent lifetime
    for (const auto &bootstrap_node : bootstrap_nodes_) {
      kademlia_->addPeer(bootstrap_node, true);
    }

    // Enqueue last active peers as first peers set but with limited lifetime
    auto last_active_peers = loadLastActivePeers();
    SL_DEBUG(log_,
             "Loaded {} last active peers' record(s)",
             last_active_peers.size());
    for (const auto &peer_info : last_active_peers) {
      kademlia_->addPeer(peer_info, false);
    }

    // Start Kademlia (processing incoming message and random walking)
    kademlia_->start();

    // Do first alignment of peers count
    align();

    return true;
  }

  void PeerManagerImpl::stop() {
    storeActivePeers();
    add_peer_handle_.unsubscribe();
    peer_disconnected_handler_.unsubscribe();
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

  void PeerManagerImpl::setCollating(
      const PeerId &peer_id,
      const network::CollatorPublicKey &collator_id,
      network::ParachainId para_id) {
    if (auto it = peer_states_.find(peer_id); it != peer_states_.end()) {
      it->second.collator_state = CollatingPeerState{
          .para_id = para_id,
          .collator_id = collator_id,
          .advertisements = {},
          .last_active = std::chrono::system_clock::now(),
      };
      it->second.time = clock_->now();
    }
  }

  void PeerManagerImpl::forOnePeer(
      const PeerId &peer_id, std::function<void(const PeerId &)> func) const {
    if (active_peers_.contains(peer_id)) {
      func(peer_id);
    }
  }

  outcome::result<
      std::pair<const network::CollatorPublicKey &, network::ParachainId>>
  PeerManagerImpl::retrieveCollatorData(
      PeerState &peer_state, const primitives::BlockHash &relay_parent) {
    if (!peer_state.collator_state) {
      return Error::UNDECLARED_COLLATOR;
    }
    return std::make_pair(peer_state.collator_state.value().collator_id,
                          peer_state.collator_state.value().para_id);
  }

  void PeerManagerImpl::align() {
    SL_TRACE(log_, "Try to align peers number");

    const auto hard_limit = app_config_.inPeers() + app_config_.inPeersLight()
                          + app_config_.outPeers();
    const auto peer_ttl = app_config_.peeringConfig().peerTtl;

    align_timer_.reset();

    clearClosedPingingConnections();

    using PriorityType = int32_t;
    using ItemType = std::pair<PriorityType, PeerId>;

    std::vector<ItemType> peers_list;
    peers_list.reserve(active_peers_.size());

    const uint64_t now_ms =
        std::chrono::time_point_cast<std::chrono::milliseconds>(clock_->now())
            .time_since_epoch()
            .count();
    const uint64_t idle_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(peer_ttl).count();

    for (const auto &[peer_id, desc] : active_peers_) {
      // Skip peer having immunity
      if (not can_disconnect_.get()->canDisconnect(peer_id)) {
        continue;
      }

      const uint64_t last_activity_ms =
          std::chrono::time_point_cast<std::chrono::milliseconds>(
              desc.time_point)
              .time_since_epoch()
              .count();

      // TODO(turuslan): #1419 disconnect peers when all relevant components
      // update peer activity time with `keepAlive`.
      [[maybe_unused]] bool activity_timeout =
          last_activity_ms + idle_ms < now_ms;

      // disconnect from peers with negative reputation
      const auto peer_reputation = reputation_repository_->reputation(peer_id);
      if (peer_reputation < kDisconnectReputation) {
        peers_list.emplace_back(std::numeric_limits<PriorityType>::min(),
                                peer_id);
        // we have to store peers somewhere first due to inability to iterate
        // over active_peers_ and do disconnectFromPeers (which modifies
        // active_peers_) at the same time
      } else {
        peers_list.emplace_back(peer_reputation, peer_id);
      }
    }

    std::ranges::sort(peers_list, [](const auto &l, const auto &r) {
      return r.first < l.first;
    });

    for (; !peers_list.empty()
           && (peers_list.size() > hard_limit
               || peers_list.back().first
                      == std::numeric_limits<PriorityType>::min());
         peers_list.pop_back()) {
      const auto &peer_id = peers_list.back().second;
      disconnectFromPeer(peer_id);
    }

    // Not enough active peers
    if (countPeers(PeerType::PEER_TYPE_OUT) < app_config_.outPeers()) {
      if (not queue_to_connect_.empty()) {
        for (;;) {
          auto node = peers_in_queue_.extract(queue_to_connect_.front());
          auto &peer_id = node.value();

          queue_to_connect_.pop_front();
          BOOST_ASSERT(queue_to_connect_.size() == peers_in_queue_.size());

          if (connecting_peers_.emplace(peer_id).second) {
            connectToPeer(peer_id);

            SL_TRACE(log_,
                     "Remained peers in queue for connect: {}",
                     peers_in_queue_.size());
            break;
          }
        }
      } else if (connecting_peers_.empty()) {
        SL_DEBUG(log_, "Queue for connect is empty. Reuse bootstrap nodes");
        for (const auto &bootstrap_node : bootstrap_nodes_) {
          if (own_peer_info_.id != bootstrap_node.id) {
            if (connecting_peers_.emplace(bootstrap_node.id).second) {
              connectToPeer(bootstrap_node.id);
            }
          }
        }
      } else {
        SL_DEBUG(log_,
                 "Queue for connect is empty. Connecting peers: {}",
                 connecting_peers_.size());
      }
    }

    const auto aligning_period = app_config_.peeringConfig().aligningPeriod;

    align_timer_ = scheduler_->scheduleWithHandle(
        [wp{weak_from_this()}] {
          if (auto self = wp.lock()) {
            self->align();
          }
        },
        aligning_period);
    SL_DEBUG(log_, "Active peers = {}", active_peers_.size());
  }

  void PeerManagerImpl::connectToPeer(const PeerId &peer_id) {
    // Skip connection to itself
    if (isSelfPeer(peer_id)) {
      connecting_peers_.erase(peer_id);
      return;
    }

    // Don't establish connection to bad (negative reputation) peers
    const auto peer_reputation = reputation_repository_->reputation(peer_id);
    if (peer_reputation < kMinReputationForOuterConnection) {
      SL_DEBUG(log_,
               "Attempt to establish connection to peer {} skipped: "
               "peer has low ({}) reputation",
               peer_id,
               peer_reputation);
      connecting_peers_.erase(peer_id);
      return;
    }

    auto peer_info = host_.getPeerRepository().getPeerInfo(peer_id);
    if (peer_info.addresses.empty()) {
      SL_DEBUG(log_, "Not found addresses for peer {}", peer_id);
      connecting_peers_.erase(peer_id);
      return;
    }

    auto connectedness = host_.connectedness(peer_info);
    if (connectedness == libp2p::Host::Connectedness::CAN_NOT_CONNECT) {
      SL_DEBUG(log_, "Can not connect to peer {}", peer_id);
      connecting_peers_.erase(peer_id);
      return;
    }

    SL_DEBUG(log_, "Try to connect to peer {}", peer_info.id);
    for (auto &addr : peer_info.addresses) {
      SL_DEBUG(log_, "  address: {}", addr.getStringAddress());
    }

    host_.connect(
        peer_info,
        [wp{weak_from_this()}, peer_id](auto res) mutable {
          auto self = wp.lock();
          if (not self) {
            return;
          }

          if (not res.has_value()) {
            SL_DEBUG(self->log_,
                     "Connecting to peer {} is failed: {}",
                     peer_id,
                     res.error());
            self->connecting_peers_.erase(peer_id);
            return;
          }

          auto &connection = res.value();
          auto remote_peer_id_res = connection->remotePeer();
          if (not remote_peer_id_res.has_value()) {
            SL_DEBUG(
                self->log_,
                "Connected, but not identified yet (expecting peer_id={:l})",
                peer_id);
            self->connecting_peers_.erase(peer_id);
            return;
          }

          auto &remote_peer_id = remote_peer_id_res.value();
          if (remote_peer_id == peer_id) {
            SL_DEBUG(self->log_, "Connected to peer {}", peer_id);

            self->processFullyConnectedPeer(peer_id);
          }
        },
        kTimeoutForConnecting);
  }

  void PeerManagerImpl::disconnectFromPeer(const PeerId &peer_id) {
    if (peer_id == own_peer_info_.id) {
      return;
    }

    SL_DEBUG(log_, "Disconnect from peer {}", peer_id);
    host_.disconnect(peer_id);
  }

  void PeerManagerImpl::keepAlive(const PeerId &peer_id) {
    auto it = active_peers_.find(peer_id);
    if (it != active_peers_.end()) {
      it->second.time_point = clock_->now();
    }
  }

  void PeerManagerImpl::startPingingPeer(const PeerId &peer_id) {
    auto ping_protocol = router_->getPingProtocol();
    BOOST_ASSERT_MSG(ping_protocol, "Router did not provide ping protocol");

    auto conn =
        host_.getNetwork().getConnectionManager().getBestConnectionForPeer(
            peer_id);
    if (conn == nullptr) {
      SL_DEBUG(log_,
               "Failed to start pinging {}: No connection to this peer exists",
               peer_id);
      return;
    }
    clearClosedPingingConnections();
    auto [_, is_emplaced] = pinging_connections_.emplace(conn);
    if (not is_emplaced) {
      // Pinging is already going
      return;
    }

    SL_DEBUG(log_,
             "Start pinging of {} (conn={})",
             peer_id,
             static_cast<void *>(conn.get()));

    ping_protocol->startPinging(
        conn,
        [wp{weak_from_this()}, peer_id, conn](
            outcome::result<std::shared_ptr<
                libp2p::protocol::PingClientSession>> session_res) {
          if (auto self = wp.lock()) {
            if (session_res.has_error()) {
              SL_DEBUG(self->log_,
                       "Stop pinging of {} (conn={}): {}",
                       peer_id,
                       static_cast<void *>(conn.get()),
                       session_res.error());
              self->pinging_connections_.erase(conn);
              self->disconnectFromPeer(peer_id);
            } else {
              SL_DEBUG(self->log_,
                       "Pinging: {} (conn={}) is alive",
                       peer_id,
                       static_cast<void *>(conn.get()));
              self->keepAlive(peer_id);
            }
          }
        });
  }

  std::optional<std::reference_wrapper<PeerState>>
  PeerManagerImpl::createDefaultPeerState(const PeerId &peer_id) {
    auto &state = peer_states_[peer_id];
    state.time = clock_->now();
    return state;
  }

  void PeerManagerImpl::updatePeerState(
      const PeerId &peer_id, const BlockAnnounceHandshake &handshake) {
    auto &state = peer_states_[peer_id];
    state.time = clock_->now();
    state.roles = handshake.roles;
    state.best_block = handshake.best_block;
  }

  void PeerManagerImpl::updatePeerState(const PeerId &peer_id,
                                        const BlockAnnounce &announce) {
    auto &state = peer_states_[peer_id];
    state.time = clock_->now();
    state.best_block = announce.header.blockInfo();
  }

  void PeerManagerImpl::updatePeerState(
      const PeerId &peer_id, const GrandpaNeighborMessage &neighbor_message) {
    auto &state = peer_states_[peer_id];
    state.time = clock_->now();
    state.round_number = neighbor_message.round_number;
    state.set_id = neighbor_message.voter_set_id;
    state.last_finalized = neighbor_message.last_finalized;
  }

  std::optional<std::reference_wrapper<PeerState>>
  PeerManagerImpl::getPeerState(const PeerId &peer_id) {
    auto it = peer_states_.find(peer_id);
    if (it == peer_states_.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  void PeerManagerImpl::enumeratePeerState(const PeersCallback &callback) {
    if (nullptr != callback) {
      for (auto &[peer, state] : peer_states_) {
        if (!callback(peer, state)) {
          break;
        }
      }
    }
  }

  std::optional<std::reference_wrapper<const PeerState>>
  PeerManagerImpl::getPeerState(const PeerId &peer_id) const {
    auto it = peer_states_.find(peer_id);
    if (it == peer_states_.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  void PeerManagerImpl::processDiscoveredPeer(const PeerId &peer_id) {
    // Ignore himself
    if (isSelfPeer(peer_id)) {
      return;
    }

    // Skip if peer is already active
    if (active_peers_.find(peer_id) != active_peers_.end()) {
      return;
    }

    auto [it, added] = peers_in_queue_.emplace(peer_id);

    // Already in queue
    if (not added) {
      return;
    }

    queue_to_connect_.emplace_back(*it);
    BOOST_ASSERT(queue_to_connect_.size() == peers_in_queue_.size());

    SL_DEBUG(log_,
             "New peer_id enqueued: {:l}. In queue: {}",
             peer_id,
             queue_to_connect_.size());
  }

  void PeerManagerImpl::processFullyConnectedPeer(const PeerId &peer_id) {
    // Skip connection to itself
    if (isSelfPeer(peer_id)) {
      connecting_peers_.erase(peer_id);
      return;
    }

    auto connection =
        host_.getNetwork().getConnectionManager().getBestConnectionForPeer(
            peer_id);
    if (connection == nullptr) {
      connecting_peers_.erase(peer_id);
      return;
    }

    // Don't accept connection from bad (negative reputation) peers
    const auto peer_reputation = reputation_repository_->reputation(peer_id);
    if (peer_reputation < kMinReputationForInnerConnection) {
      SL_DEBUG(log_,
               "New connection from peer {} was dropped: "
               "peer has low ({}) reputation",
               peer_id,
               peer_reputation);
      connecting_peers_.erase(peer_id);
      disconnectFromPeer(peer_id);
      return;
    }

    PeerInfo peer_info{.id = peer_id, .addresses = {}};

    auto addresses_res =
        host_.getPeerRepository().getAddressRepository().getAddresses(peer_id);
    if (addresses_res.has_value()) {
      auto &addresses = addresses_res.value();
      peer_info.addresses = std::move(addresses);
      kademlia_->addPeer(peer_info, false);
    }
  }

  bool PeerManagerImpl::isSelfPeer(const PeerId &peer_id) const {
    return own_peer_info_.id == peer_id;
  }

  std::vector<scale::PeerInfoSerializable>
  PeerManagerImpl::loadLastActivePeers() {
    auto get_res = storage_->get(storage::kActivePeersKey);
    if (not get_res) {
      SL_ERROR(log_,
               "List of last active peers cannot be obtained from storage. "
               "Error={}",
               get_res.error());
      return {};
    }

    std::vector<scale::PeerInfoSerializable> last_active_peers;
    scale::ScaleDecoderStream s{get_res.value()};
    try {
      s >> last_active_peers;
    } catch (...) {
      SL_ERROR(log_, "Unable to decode list of active peers");
      return {};
    }
    return last_active_peers;
  }

  void PeerManagerImpl::storeActivePeers() {
    std::vector<libp2p::peer::PeerInfo> last_active_peers;
    for (const auto &peer_id : recently_active_peers_) {
      auto peer_info = host_.getPeerRepository().getPeerInfo(peer_id);
      last_active_peers.push_back(peer_info);
    }

    if (last_active_peers.empty()) {
      SL_DEBUG(log_,
               "Zero last active peers, won't save zero. Storage will remain "
               "untouched.");
      return;
    }

    scale::ScaleEncoderStream out;
    try {
      out << last_active_peers;
    } catch (...) {
      SL_ERROR(log_, "Unable to encode list of active peers");
      return;
    }

    auto save_res = storage_->put(storage::kActivePeersKey,
                                  common::Buffer{out.to_vector()});
    if (not save_res) {
      SL_ERROR(log_, "Cannot store active peers. Error={}", save_res.error());
      return;
    }
    SL_DEBUG(log_,
             "Saved {} last active peers' record(s)",
             last_active_peers.size());
  }

  void PeerManagerImpl::clearClosedPingingConnections() {
    for (auto it = pinging_connections_.begin();
         it != pinging_connections_.end();) {
      if ((**it).isClosed()) {
        it = pinging_connections_.erase(it);
      } else {
        ++it;
      }
    }
  }

  size_t PeerManagerImpl::countPeers(PeerType in_out, IsLight in_light) const {
    return std::ranges::count_if(
        active_peers_, [&](const decltype(active_peers_)::value_type &x) {
          if (x.second.peer_type == PeerType::PEER_TYPE_OUT) {
            return in_out == PeerType::PEER_TYPE_OUT;
          }
          if (in_out == PeerType::PEER_TYPE_OUT) {
            return false;
          }
          auto it = peer_states_.find(x.first);
          if (it == peer_states_.end()) {
            return false;
          }
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
          const auto &roles = it->second.roles.flags;
          return (in_light ? roles.light : roles.full) == 1;
        });
  }

  std::optional<PeerId> PeerManagerImpl::peerFinalized(
      BlockNumber min, const PeerPredicate &predicate) {
    for (auto &[peer, info] : peer_states_) {
      if (info.last_finalized < min) {
        continue;
      }
      if (not predicate or predicate(peer)) {
        return peer;
      }
    }
    return std::nullopt;
  }
}  // namespace kagome::network
