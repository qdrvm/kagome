/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_GOSSIPER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_GOSSIPER_MOCK_HPP

#include "consensus/grandpa/gossiper.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  class GossiperMock : public Gossiper {
   public:
    MOCK_METHOD2(catchUpRequest,
                 void(const libp2p::peer::PeerId &,
                      const network::CatchUpRequest &));
    MOCK_METHOD2(catchUpResponse,
                 void(const libp2p::peer::PeerId &,
                      const network::CatchUpResponse &));
    MOCK_METHOD1(vote, void(const network::GrandpaVote &));
    MOCK_METHOD1(finalize, void(const network::GrandpaCommit &));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_GOSSIPER_MOCK_HPP
