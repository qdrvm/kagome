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
    MOCK_METHOD1(setExecutionStrategy, void(ExecutionStrategy strategy));
    MOCK_METHOD2(runEpoch,
                 void(Epoch epoch, BabeTimePoint starting_slot_finish_time));
    MOCK_CONST_METHOD0(getCurrentState, State());

    MOCK_METHOD1(doOnSynchronized, void(std::function<void()> handler));

    MOCK_METHOD2(onBlockAnnounce,
                 void(const libp2p::peer::PeerId &peer_id,
                      const network::BlockAnnounce &announce));
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABE_MOCK_HPP
