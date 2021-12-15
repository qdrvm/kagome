/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_MOCK_HPP
#define KAGOME_CONSENSUS_BABE_MOCK_HPP

#include "consensus/babe/babe.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::babe {

  class BabeMock final : public Babe {
   public:
    MOCK_METHOD(void, runEpoch, (EpochDescriptor epoch), (override));

    MOCK_METHOD(State, getCurrentState, (), (const, override));

    MOCK_METHOD(void, doOnSynchronized, (std::function<void()> handler));

    MOCK_METHOD(void,
                onRemoteStatus,
                (const libp2p::peer::PeerId &peer_id,
                 const network::Status &status),
                (override));

    MOCK_METHOD(void,
                onBlockAnnounce,
                (const libp2p::peer::PeerId &peer_id,
                 const network::BlockAnnounce &announce),
                (override));

    MOCK_METHOD(void, onSynchronized, (), (override));

    MOCK_METHOD(bool, wasSynchronized, (), (const, override));
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABE_MOCK_HPP
