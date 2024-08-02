/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/peer_manager.hpp"

#include <memory>
#include <queue>

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/event/bus.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/protocol/identify/identify.hpp>
#include <libp2p/protocol/kademlia/kademlia.hpp>

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "application/chain_spec.hpp"
#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree.hpp"
#include "clock/clock.hpp"
#include "consensus/grandpa/voting_round.hpp"
#include "crypto/hasher.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "network/impl/protocols/block_announce_protocol.hpp"
#include "network/impl/protocols/propagate_transactions_protocol.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/peer_view.hpp"
#include "network/protocols/sync_protocol.hpp"
#include "network/reputation_repository.hpp"
#include "network/router.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/bootstrap_nodes.hpp"
#include "network/types/own_peer_info.hpp"
#include "scale/libp2p_types.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome {
  class PoolHandlerReady;
}

namespace kagome::network {
  class CanDisconnect;

  enum class PeerType { PEER_TYPE_IN = 0, PEER_TYPE_OUT };

  struct PeerDescriptor {
    PeerType peer_type;
    clock::SteadyClock::TimePoint time_point;
  };

  class PeerManagerImpl : public PeerManager,
                          public std::enable_shared_from_this<PeerManagerImpl> {
   public:
    static constexpr std::chrono::seconds kTimeoutForConnecting{15};

    enum class Error { UNDECLARED_COLLATOR = 1 };

    PeerManagerImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        libp2p::Host &host,
        common::MainThreadPool &main_thread_pool,
        std::shared_ptr<libp2p::protocol::Identify> identify,
        std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler,
        std::shared_ptr<StreamEngine> stream_engine,
        const application::AppConfiguration &app_config,
        std::shared_ptr<clock::SteadyClock> clock,
        const BootstrapNodes &bootstrap_nodes,
        const OwnPeerInfo &own_peer_info,
        std::shared_ptr<network::Router> router,
        std::shared_ptr<storage::SpacedStorage> storage,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<ReputationRepository> reputation_repository,
        LazySPtr<CanDisconnect> can_disconnect,
        std::shared_ptr<PeerView> peer_view);

    /** @see poolHandlerReadyMake */
    bool tryStart();

    /** @see AppStateManager::takeControl */
    void stop();

    /** @see PeerManager::connectToPeer */
    void connectToPeer(const PeerInfo &peer_info) override;

    /** @see PeerManager::reserveStreams */
    void reserveStreams(const PeerId &peer_id) const override;

    /** @see PeerManager::reserveStatusStreams */
    void reserveStatusStreams(const PeerId &peer_id) const override;

    /** @see PeerManager::getStreamEngine */
    std::shared_ptr<StreamEngine> getStreamEngine() override;

    /** @see PeerManager::activePeersNumber */
    size_t activePeersNumber() const override;

    /** @see PeerManager::setCollating */
    void setCollating(const PeerId &peer_id,
                      const network::CollatorPublicKey &collator_id,
                      network::ParachainId para_id) override;

    outcome::result<
        std::pair<const network::CollatorPublicKey &, network::ParachainId>>
    retrieveCollatorData(PeerState &peer_state,
                         const primitives::BlockHash &relay_parent) override;

    /** @see PeerManager::forEachPeer */
    void forEachPeer(std::function<void(const PeerId &)> func) const override;

    /** @see PeerManager::forOnePeer */
    void forOnePeer(const PeerId &peer_id,
                    std::function<void(const PeerId &)> func) const override;

    /** @see PeerManager::forOnePeer */
    void keepAlive(const PeerId &peer_id) override;

    /** @see PeerManager::startPingingPeer */
    void startPingingPeer(const PeerId &peer_id) override;

    /** @see PeerManager::updatePeerState */
    void updatePeerState(const PeerId &peer_id,
                         const BlockAnnounceHandshake &handshake) override;

    /** @see PeerManager::updatePeerState */
    void updatePeerState(const PeerId &peer_id,
                         const BlockAnnounce &announce) override;

    std::optional<std::reference_wrapper<PeerState>> createDefaultPeerState(
        const PeerId &peer_id) override;

    /** @see PeerManager::updatePeerState */
    void updatePeerState(
        const PeerId &peer_id,
        const GrandpaNeighborMessage &neighbor_message) override;

    /** @see PeerManager::getPeerState */
    std::optional<std::reference_wrapper<PeerState>> getPeerState(
        const PeerId &peer_id) override;
    std::optional<std::reference_wrapper<const PeerState>> getPeerState(
        const PeerId &peer_id) const override;

    void enumeratePeerState(const PeersCallback &callback) override;

    std::optional<PeerId> peerFinalized(
        BlockNumber min, const PeerPredicate &predicate) override;

   private:
    /// Right way to check self peer as it takes into account dev mode
    bool isSelfPeer(const PeerId &peer_id) const;

    /// Aligns amount of connected streams
    void align();

    void processDiscoveredPeer(const PeerId &peer_id);

    void processFullyConnectedPeer(const PeerId &peer_id);

    template <typename F>
    void openBlockAnnounceProtocol(
        const PeerInfo &peer_info,
        const libp2p::network::ConnectionManager::ConnectionSPtr &connection,
        F &&opened_callback);
    void tryOpenGrandpaProtocol(const PeerInfo &peer_info,
                                PeerState &peer_state);
    void tryOpenValidationProtocol(const PeerInfo &peer_info,
                                   PeerState &peer_state,
                                   network::CollationVersion proto_version);

    /// Opens streams set for special peer (i.e. new-discovered)
    void connectToPeer(const PeerId &peer_id);

    /// Closes all streams of provided peer
    void disconnectFromPeer(const PeerId &peer_id);

    std::vector<scale::PeerInfoSerializable> loadLastActivePeers();

    /// Stores peers addresses to the state storage to warm up the following
    /// node start
    void storeActivePeers();

    void clearClosedPingingConnections();

    using IsLight = Tagged<bool, struct IsLightTag>;
    size_t countPeers(PeerType in_out, IsLight in_light = false) const;

    log::Logger log_;

    libp2p::Host &host_;
    std::shared_ptr<PoolHandlerReady> main_pool_handler_;
    std::shared_ptr<libp2p::protocol::Identify> identify_;
    std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::shared_ptr<StreamEngine> stream_engine_;
    const application::AppConfiguration &app_config_;
    std::shared_ptr<clock::SteadyClock> clock_;
    const BootstrapNodes &bootstrap_nodes_;
    const OwnPeerInfo &own_peer_info_;
    std::shared_ptr<network::Router> router_;
    std::shared_ptr<storage::BufferStorage> storage_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<ReputationRepository> reputation_repository_;
    LazySPtr<CanDisconnect> can_disconnect_;
    std::shared_ptr<network::PeerView> peer_view_;

    libp2p::event::Handle add_peer_handle_;
    libp2p::event::Handle peer_disconnected_handler_;
    std::unordered_set<PeerId> peers_in_queue_;
    std::deque<PeerId> queue_to_connect_;
    std::unordered_set<PeerId> connecting_peers_;
    std::unordered_set<libp2p::network::ConnectionManager::ConnectionSPtr>
        pinging_connections_;

    std::map<PeerId, PeerDescriptor> active_peers_;
    std::unordered_map<PeerId, PeerState> peer_states_;
    libp2p::basic::Scheduler::Handle align_timer_;
    std::set<PeerId> recently_active_peers_;

    // metrics
    metrics::RegistryPtr registry_ = metrics::createRegistry();
    metrics::Gauge *sync_peer_num_;
    metrics::Gauge *peers_count_metric_;
  };

}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, PeerManagerImpl::Error)
