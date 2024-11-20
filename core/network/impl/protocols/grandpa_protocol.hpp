/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <random>
#include <set>

#include "log/logger.hpp"
#include "network/notifications/protocol.hpp"
#include "network/types/grandpa_message.hpp"
#include "network/types/roles.hpp"
#include "utils/lru.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::blockchain {
  class GenesisBlockHash;
}  // namespace kagome::blockchain

namespace kagome::consensus::grandpa {
  class GrandpaObserver;
}  // namespace kagome::consensus::grandpa

namespace kagome::crypto {
  class Hasher;
}  // namespace kagome::crypto

namespace kagome::network {
  struct OwnPeerInfo;
  class PeerManager;
}  // namespace kagome::network

namespace kagome::network {
  using libp2p::PeerId;

  class GrandpaProtocol final
      : public std::enable_shared_from_this<GrandpaProtocol>,
        public notifications::Controller,
        NonCopyable,
        NonMovable {
   public:
    GrandpaProtocol(
        const notifications::Factory &notifications_factory,
        std::shared_ptr<crypto::Hasher> hasher,
        Roles roles,
        std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer,
        const OwnPeerInfo &own_info,
        std::shared_ptr<PeerManager> peer_manager,
        const blockchain::GenesisBlockHash &genesis_hash,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler);

    // Controller
    Buffer handshake() override;
    bool onHandshake(const PeerId &peer_id,
                     size_t,
                     bool out,
                     Buffer &&handshake) override;
    bool onMessage(const PeerId &peer_id, size_t, Buffer &&message) override;
    void onClose(const PeerId &peer_id) override;

    void start();

    void vote(network::GrandpaVote &&vote_message,
              std::optional<const libp2p::peer::PeerId> peer_id);
    void neighbor(GrandpaNeighborMessage &&msg);
    void finalize(FullCommitMessage &&msg,
                  std::optional<const libp2p::peer::PeerId> peer_id);
    void catchUpRequest(const libp2p::peer::PeerId &peer_id,
                        CatchUpRequest &&catch_up_request);
    void catchUpResponse(const libp2p::peer::PeerId &peer_id,
                         CatchUpResponse &&catch_up_response);

   private:
    static constexpr auto kGrandpaProtocolName = "GrandpaProtocol";

    struct RawMessage {
      std::shared_ptr<Buffer> raw;
      std::optional<Hash256> hash;
    };
    std::optional<Hash256> rawMessageHash(const GrandpaMessage &message,
                                          BufferView message_raw) const;
    RawMessage rawMessage(const GrandpaMessage &message) const;
    bool write(const PeerId &peer_id, RawMessage message);
    template <typename F>
    void broadcast(const RawMessage &message, const F &predicate);

    /// Node should send catch-up requests rarely to be polite, because
    /// processing of them consume more enough resources.
    /// How long replying outgoing catch-up requests must be suppressed
    static constexpr std::chrono::milliseconds kRecentnessDuration =
        std::chrono::seconds(300);

    log::Logger log_;
    std::shared_ptr<notifications::Protocol> notifications_;
    std::shared_ptr<crypto::Hasher> hasher_;
    Roles roles_;
    std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer_;
    const OwnPeerInfo &own_info_;
    std::shared_ptr<PeerManager> peer_manager_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    MapLruSet<PeerId, Hash256> seen_;

    std::set<std::tuple<consensus::grandpa::RoundNumber,
                        consensus::grandpa::VoterSetId>>
        recent_catchup_requests_by_round_;

    GrandpaNeighborMessage last_neighbor_{};

    std::set<libp2p::peer::PeerId> recent_catchup_requests_by_peer_;

    std::default_random_engine random_;

    friend class BlockAnnounceProtocol;
  };

}  // namespace kagome::network
