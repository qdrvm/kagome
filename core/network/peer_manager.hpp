/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>

#include "network/peer_state.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/block_announce_handshake.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "network/types/grandpa_message.hpp"
#include "outcome/outcome.hpp"
#include "parachain/validator/backing_implicit_view.hpp"
#include "parachain/validator/collations.hpp"
#include "primitives/common.hpp"
#include "utils/lru.hpp"

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
    using PeerPredicate = std::function<bool(const PeerId &)>;
    using PeersCallback = std::function<bool(const PeerId &, PeerState &)>;

    virtual ~PeerManager() = default;

    /**
     * Starts outgoing pinging to peer with {@param peer_id}
     */
    virtual void startPingingPeer(const PeerId &peer_id) = 0;

    /**
     * Updates known data about peer with {@param peer_id} by {@param handshake}
     */
    virtual void updatePeerState(const PeerId &peer_id,
                                 const BlockAnnounceHandshake &handshake) = 0;

    /**
     * Updates known data about peer with {@param peer_id} by {@param announce}
     */
    virtual void updatePeerState(const PeerId &peer_id,
                                 const BlockAnnounce &announce) = 0;

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
     * Apply callback to each PeerState.
     */
    virtual void enumeratePeerState(const PeersCallback &callback) = 0;

    /**
     * @returns number of active peers
     */
    virtual size_t activePeersNumber() const = 0;

    /**
     * Find peer that have already finalized specified block.
     * Used by `SynchronizerImpl` and `BeefyImpl` to fetch justifications.
     */
    virtual std::optional<PeerId> peerFinalized(
        BlockNumber min, const PeerPredicate &predicate) = 0;

    /**
     * Get grandpa specific peer information.
     */
    virtual std::optional<PeerStateCompact> getGrandpaInfo(
        const PeerId &peer_id) = 0;
    /**
     * Set peer collation protocol version.
     */
    virtual std::optional<CollationVersion> getCollationVersion(
        const PeerId &peer_id) = 0;
    /**
     * Get peer collation protocol version.
     */
    virtual void setCollationVersion(const PeerId &peer_id,
                                     CollationVersion collation_version) = 0;
    /**
     * Get peer fetch chunk protocol version.
     */
    virtual std::optional<ReqChunkVersion> getReqChunkVersion(
        const PeerId &peer_id) = 0;
    /**
     * Set peer fetch chunk protocol version.
     */
    virtual void setReqChunkVersion(const PeerId &peer_id,
                                    ReqChunkVersion req_chunk_version) = 0;
    /**
     * Check if peer is collating.
     */
    virtual std::optional<bool> isCollating(const PeerId &peer_id) = 0;
    /**
     * Check if collation have already been advertised by peer.
     */
    virtual std::optional<bool> hasAdvertised(
        const PeerId &peer_id,
        const RelayHash &relay_parent,
        const std::optional<CandidateHash> &candidate_hash) = 0;
    using InsertAdvertisementResult =
        outcome::result<std::pair<CollatorPublicKey, ParachainId>>;
    /**
     * Insert advertisement from peer.
     */
    virtual InsertAdvertisementResult insertAdvertisement(
        const PeerId &peer_id,
        const RelayHash &on_relay_parent,
        const parachain::ProspectiveParachainsModeOpt &relay_parent_mode,
        const std::optional<std::reference_wrapper<const CandidateHash>>
            &candidate_hash) = 0;
  };
}  // namespace kagome::network
