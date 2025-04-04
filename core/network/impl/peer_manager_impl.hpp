/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/peer_manager.hpp"

#include <memory>
#include <mutex>
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
#include "network/impl/protocols/propagate_transactions_protocol.hpp"
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
    enum class Error { UNDECLARED_COLLATOR = 1 };

    PeerManagerImpl(
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
        std::shared_ptr<PeerView> peer_view,
        primitives::events::PeerSubscriptionEnginePtr peer_event_engine);

    /** @see poolHandlerReadyMake */
    bool tryStart();

    /** @see AppStateManager::takeControl */
    void stop();

    /** @see PeerManager::activePeersNumber */
    size_t activePeersNumber() const override;

    /** @see PeerManager::setCollating */
    void setCollating(const PeerId &peer_id,
                      const network::CollatorPublicKey &collator_id,
                      network::ParachainId para_id) override;

    /** @see PeerManager::startPingingPeer */
    void startPingingPeer(const PeerId &peer_id) override;

    /** @see PeerManager::updatePeerState */
    void updatePeerState(const PeerId &peer_id,
                         const BlockAnnounceHandshake &handshake) override;

    /** @see PeerManager::updatePeerState */
    void updatePeerState(const PeerId &peer_id,
                         const BlockAnnounce &announce) override;

    /** @see PeerManager::updatePeerState */
    void updatePeerState(
        const PeerId &peer_id,
        const GrandpaNeighborMessage &neighbor_message) override;

    void enumeratePeerState(const PeersCallback &callback) override;

    std::optional<PeerId> peerFinalized(
        BlockNumber min, const PeerPredicate &predicate) override;

    std::optional<PeerStateCompact> getGrandpaInfo(
        const PeerId &peer_id) override;
    std::optional<CollationVersion> getCollationVersion(
        const PeerId &peer_id) override;
    void setCollationVersion(const PeerId &peer_id,
                             CollationVersion collation_version) override;
    std::optional<ReqChunkVersion> getReqChunkVersion(
        const PeerId &peer_id) override;
    void setReqChunkVersion(const PeerId &peer_id,
                            ReqChunkVersion req_chunk_version) override;
    std::optional<bool> isCollating(const PeerId &peer_id) override;
    std::optional<bool> hasAdvertised(
        const PeerId &peer_id,
        const RelayHash &relay_parent,
        const std::optional<CandidateHash> &candidate_hash) override;
    std::optional<ParachainId> getParachainId(const PeerId &peer_id) override;
    InsertAdvertisementResult insertAdvertisement(
        const PeerId &peer_id,
        const RelayHash &on_relay_parent,
        const parachain::ProspectiveParachainsModeOpt &relay_parent_mode,
        const std::optional<std::reference_wrapper<const CandidateHash>>
            &candidate_hash) override;

   private:
    void keepAlive(const PeerId &peer_id);

    /// Right way to check self peer as it takes into account dev mode
    bool isSelfPeer(const PeerId &peer_id) const;

    /// Aligns amount of connected streams
    void align();

    void processDiscoveredPeer(const PeerId &peer_id);

    void processFullyConnectedPeer(const PeerId &peer_id);

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
    size_t countPeers(PeerType in_out, IsLight in_light = IsLight{false}) const;

    void collectGarbage();

    log::Logger log_;

    libp2p::Host &host_;
    std::shared_ptr<PoolHandlerReady> main_pool_handler_;
    std::shared_ptr<libp2p::protocol::Identify> identify_;
    std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
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
    libp2p::event::Handle peer_connected_sub_;
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
    primitives::events::PeerSubscriptionEnginePtr peer_event_engine_;
    mutable std::recursive_mutex mutex_;
  };

}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, PeerManagerImpl::Error)
