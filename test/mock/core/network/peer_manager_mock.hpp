/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/peer_manager.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class PeerManagerMock final : public PeerManager {
   public:
    MOCK_METHOD(void, startPingingPeer, (const PeerId &), (override));

    MOCK_METHOD(void,
                setCollating,
                (const PeerId &,
                 const network::CollatorPublicKey &,
                 network::ParachainId),
                (override));

    MOCK_METHOD(void,
                updatePeerState,
                (const PeerId &, const BlockAnnounceHandshake &),
                (override));

    MOCK_METHOD(void,
                updatePeerState,
                (const PeerId &, const BlockAnnounce &),
                (override));

    MOCK_METHOD(void,
                updatePeerState,
                (const PeerId &, const GrandpaNeighborMessage &),
                (override));

    MOCK_METHOD(void, enumeratePeerState, (const PeersCallback &), (override));

    MOCK_METHOD(size_t, activePeersNumber, (), (const, override));

    MOCK_METHOD(std::optional<PeerId>,
                peerFinalized,
                (BlockNumber, const PeerPredicate &),
                (override));

    MOCK_METHOD(std::optional<PeerStateCompact>,
                getGrandpaInfo,
                (const PeerId &),
                (override));
    MOCK_METHOD(std::optional<CollationVersion>,
                getCollationVersion,
                (const PeerId &),
                (override));
    MOCK_METHOD(void,
                setCollationVersion,
                (const PeerId &, CollationVersion),
                (override));
    MOCK_METHOD(std::optional<ReqChunkVersion>,
                getReqChunkVersion,
                (const PeerId &),
                (override));
    MOCK_METHOD(void,
                setReqChunkVersion,
                (const PeerId &, ReqChunkVersion),
                (override));
    MOCK_METHOD(std::optional<bool>, isCollating, (const PeerId &), (override));
    MOCK_METHOD(std::optional<bool>,
                hasAdvertised,
                (const PeerId &,
                 const RelayHash &,
                 const std::optional<CandidateHash> &),
                (override));
    MOCK_METHOD(std::optional<ParachainId>,
                getParachainId,
                (const PeerId &),
                (override));
    MOCK_METHOD(
        InsertAdvertisementResult,
        insertAdvertisement,
        (const PeerId &,
         const RelayHash &,
         const parachain::ProspectiveParachainsModeOpt &,
         const std::optional<std::reference_wrapper<const CandidateHash>> &),
        (override));
  };

}  // namespace kagome::network
