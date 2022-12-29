/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PEERMANAGER
#define KAGOME_NETWORK_PEERMANAGER

#include <optional>
#include <unordered_set>

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>

#include "network/types/block_announce.hpp"
#include "network/types/collator_messages.hpp"
#include "network/types/grandpa_message.hpp"
#include "network/types/status.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  struct CollatorState {
    network::ParachainId parachain_id;
    network::CollatorPublicKey collator_id;
    std::unordered_set<BlockHash> advertisements;
  };

  struct PendingCollation {
    network::ParachainId para_id;
    BlockHash const &relay_parent;
    libp2p::peer::PeerId const &peer_id;
  };

  using OurView = network::View;

  struct PeerState {
    clock::SteadyClock::TimePoint time;
    Roles roles = 0;
    BlockInfo best_block = {0, {}};
    std::optional<RoundNumber> round_number = std::nullopt;
    std::optional<VoterSetId> set_id = std::nullopt;
    BlockNumber last_finalized = 0;
    std::optional<CollatorState> collator_state = std::nullopt;
    std::optional<View> view;
  };

  struct StreamEngine;

  /**
   * Manage active peers:
   * - peer discovery (internally)
   * - provides needed amount of active peers
   * - connects to specified peer by their id
   * - apply some func for active peer(s)
   */
  class PeerManager {
   public:
    using PeerId = libp2p::peer::PeerId;
    using PeerInfo = libp2p::peer::PeerInfo;
    using BlockInfo = primitives::BlockInfo;
    using AdvResult = outcome::result<
        std::pair<network::CollatorPublicKey const &, network::ParachainId>>;

    virtual ~PeerManager() = default;

    /**
     * Force connect to peer by {@param peer_info}
     */
    virtual void connectToPeer(const PeerInfo &peer_info) = 0;

    /**
     * Reserves stream slots of needed protocols for peer by {@param peer_id}
     */
    virtual void reserveStreams(const PeerId &peer_id) const = 0;

    /**
     * Return stream engine object.
     */
    virtual std::shared_ptr<StreamEngine> getStreamEngine() = 0;

    /**
     * Keeps peer with {@param peer_id} alive
     */
    virtual void keepAlive(const PeerId &peer_id) = 0;

    /**
     * Starts outgoing pinging to peer with {@param peer_id}
     */
    virtual void startPingingPeer(const PeerId &peer_id) = 0;

    /**
     * Updates known data about peer with {@param peer_id} by {@param status}
     */
    virtual void updatePeerState(const PeerId &peer_id,
                                 const Status &status) = 0;

    /**
     * Updates known data about peer with {@param peer_id} by {@param announce}
     */
    virtual void updatePeerState(const PeerId &peer_id,
                                 const BlockAnnounce &announce) = 0;

    /**
     * Store advertisement from a peer to later processing;
     */
    virtual outcome::result<
        std::pair<network::CollatorPublicKey const &, network::ParachainId>>
    insertAdvertisement(PeerState &peer_state,
                         primitives::BlockHash para_hash) = 0;

    /**
     * Updates collation state and stores parachain id. Should be called once
     * for each peer per connection. If else -> reduce reputation.
     */
    virtual void setCollating(const PeerId &peer_id,
                              network::CollatorPublicKey const &collator_id,
                              network::ParachainId para_id) = 0;

    /**
     * Updates known data about peer with {@param peer_id} by {@param
     * neighbor_message}
     */
    virtual void updatePeerState(
        const PeerId &peer_id,
        const GrandpaNeighborMessage &neighbor_message) = 0;

    /**
     * @returns known info about peer with {@param peer_id} or none
     */
    virtual std::optional<std::reference_wrapper<PeerState>> getPeerState(
        const PeerId &peer_id) = 0;

    /**
     * @returns number of active peers
     */
    virtual size_t activePeersNumber() const = 0;

    /**
     * Apply {@param func} to each active peer
     */
    virtual void forEachPeer(
        std::function<void(const PeerId &)> func) const = 0;

    /**
     * Apply {@param func} to an active peer with id {@param peer_id}
     */
    virtual void forOnePeer(const PeerId &peer_id,
                            std::function<void(const PeerId &)> func) const = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PEERMANAGER
