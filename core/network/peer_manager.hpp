/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <unordered_set>

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>

#include "network/peer_state.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/block_announce_handshake.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "network/types/grandpa_message.hpp"
#include "outcome/outcome.hpp"
#include "parachain/validator/backing_implicit_view.hpp"
#include "primitives/common.hpp"
#include "utils/lru.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {
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
        std::pair<const network::CollatorPublicKey &, network::ParachainId>>;
    using PeerPredicate = std::function<bool(const PeerId &)>;
    using PeersCallback = std::function<bool(const PeerId &, PeerState &)>;

    virtual ~PeerManager() = default;

    /**
     * Force connect to peer by {@param peer_info}
     */
    virtual void connectToPeer(const PeerInfo &peer_info) = 0;

    /**
     * Keeps peer with {@param peer_id} alive
     */
    virtual void keepAlive(const PeerId &peer_id) = 0;

    /**
     * Starts outgoing pinging to peer with {@param peer_id}
     */
    virtual void startPingingPeer(const PeerId &peer_id) = 0;

    /**
     * Updates known data about peer with {@param peer_id} by {@param handshake}
     */
    virtual void updatePeerState(const PeerId &peer_id,
                                 const BlockAnnounceHandshake &handshake) = 0;

    virtual std::optional<std::reference_wrapper<PeerState>>
    createDefaultPeerState(const PeerId &peer_id) = 0;

    /**
     * Updates known data about peer with {@param peer_id} by {@param announce}
     */
    virtual void updatePeerState(const PeerId &peer_id,
                                 const BlockAnnounce &announce) = 0;

    /**
     * Store advertisement from a peer to later processing;
     */
    virtual outcome::result<
        std::pair<const network::CollatorPublicKey &, network::ParachainId>>
    retrieveCollatorData(PeerState &peer_state,
                         const primitives::BlockHash &relay_parent) = 0;

    /**
     * Updates collation state and stores parachain id. Should be called once
     * for each peer per connection. If else -> reduce reputation.
     */
    virtual void setCollating(const PeerId &peer_id,
                              const network::CollatorPublicKey &collator_id,
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
    virtual std::optional<std::reference_wrapper<const PeerState>> getPeerState(
        const PeerId &peer_id) const = 0;

    /**
     * Apply callback to each PeerState.
     */
    virtual void enumeratePeerState(const PeersCallback &callback) = 0;

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

    /**
     * Find peer that have already finalized specified block.
     * Used by `SynchronizerImpl` and `BeefyImpl` to fetch justifications.
     */
    virtual std::optional<PeerId> peerFinalized(
        BlockNumber min, const PeerPredicate &predicate) = 0;
  };
}  // namespace kagome::network
