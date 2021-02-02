/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PEERMANAGERIMPL
#define KAGOME_NETWORK_PEERMANAGERIMPL

#include "network/peer_manager.hpp"

#include <memory>
#include <queue>

#include <libp2p/event/bus.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/protocol/identify/identify.hpp>
#include <libp2p/protocol/kademlia/kademlia.hpp>

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "application/chain_spec.hpp"
#include "clock/clock.hpp"
#include "common/logger.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/types/bootstrap_nodes.hpp"
#include "network/types/own_peer_info.hpp"

namespace kagome::network {

  class PeerManagerImpl : public PeerManager,
                          public std::enable_shared_from_this<PeerManagerImpl> {
   public:
    PeerManagerImpl(
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
        const OwnPeerInfo &own_peer_info);

    /** @see AppStateManager::takeControl */
    bool prepare();

    /** @see AppStateManager::takeControl */
    bool start();

    /** @see AppStateManager::takeControl */
    void stop();

    /** @see PeerManager::connectToPeer */
    void connectToPeer(const PeerInfo &peer_info) override;

    /** @see PeerManager::forEachPeer */
    void forEachPeer(std::function<void(const PeerId &)> func) const override;

    /** @see PeerManager::forOnePeer */
    void forOnePeer(const PeerId &peer_id,
                    std::function<void()> func) const override;

    /** @see PeerManager::forOnePeer */
    void keepAlive(const PeerId &peer_id) override;

   private:
    /// Aligns amount of connected streams
    void align();

    /// Opens streams set for special peer (i.e. new-discovered)
    void connectToPeer(const PeerId &peer_id);

    /// Closes all streams of provided peer
    void disconnectFromPeer(const PeerId &peer_id);

    std::shared_ptr<application::AppStateManager> app_state_manager_;
    libp2p::Host &host_;
    std::shared_ptr<libp2p::protocol::Identify> identify_;
    std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia_;
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;
    std::shared_ptr<StreamEngine> stream_engine_;
    const application::AppConfiguration &app_config_;
    const application::ChainSpec &chain_spec_;
    const clock::SteadyClock &clock_;
    const BootstrapNodes &bootstrap_nodes_;
    const OwnPeerInfo &own_peer_info_;

    libp2p::event::Handle add_peer_handle_;
    std::unordered_set<PeerId, std::hash<PeerId>, std::equal_to<PeerId>>
        peers_in_queue_;
    std::deque<std::reference_wrapper<const PeerId>> queue_to_connect_;
    std::unordered_map<PeerId, clock::SteadyClock::TimePoint> active_peers_;
    libp2p::protocol::scheduler::Handle align_timer_;

    common::Logger log_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PEERMANAGERIMPL
