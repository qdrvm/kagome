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

#include "network/types/block_announce.hpp"
#include "network/types/block_announce_handshake.hpp"
#include "network/types/collator_messages.hpp"
#include "network/types/grandpa_message.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"
#include "utils/lru.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {
  constexpr size_t kPeerStateMaxKnownBlocks = 1024;
  constexpr size_t kPeerStateMaxKnownGrandpaMessages = 8192;

  struct CollatorState {
    network::ParachainId parachain_id;
    network::CollatorPublicKey collator_id;
  };

  struct PendingCollation {
    RelayHash relay_parent;
    network::ParachainId para_id;
    libp2p::peer::PeerId peer_id;
    std::optional<Hash> commitments_hash{};
  };

  struct CollationEvent {
    CollatorId collator_id;
    PendingCollation pending_collation;
  };

  using OurView = network::View;

  struct PeerStateCompact {
    std::optional<RoundNumber> round_number;
    std::optional<VoterSetId> set_id;
    BlockNumber last_finalized;
  };

  struct PeerState {
    clock::SteadyClock::TimePoint time;
    Roles roles = 0;
    BlockInfo best_block = {0, {}};
    std::optional<RoundNumber> round_number = std::nullopt;
    std::optional<VoterSetId> set_id = std::nullopt;
    BlockNumber last_finalized = 0;
    std::optional<CollatorState> collator_state = std::nullopt;
    std::optional<View> view;
    LruSet<primitives::BlockHash> known_blocks{kPeerStateMaxKnownBlocks};
    LruSet<common::Hash256> known_grandpa_messages{
        kPeerStateMaxKnownGrandpaMessages,
    };

    PeerStateCompact compact() const {
      return PeerStateCompact{
          .round_number = round_number,
          .set_id = set_id,
          .last_finalized = last_finalized,
      };
    }
  };

  inline std::optional<PeerStateCompact> compactFromRefToOwn(
      const std::optional<std::reference_wrapper<PeerState>> &opt_ref) {
    if (opt_ref) {
      return opt_ref->get().compact();
    }
    return std::nullopt;
  }

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
        std::pair<const network::CollatorPublicKey &, network::ParachainId>>;

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
     * Reserves streams needed to update our status.
     */
    virtual void reserveStatusStreams(const PeerId &peer_id) const = 0;

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
