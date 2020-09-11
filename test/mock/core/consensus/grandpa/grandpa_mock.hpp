/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_GRANDPA_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_GRANDPA_MOCK_HPP

#include "consensus/grandpa/grandpa.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  class GrandpaMock : public Grandpa {
   public:
    MOCK_METHOD0(executeNextRound, void());
    MOCK_METHOD1(onFinalize, void(const Fin &));
    MOCK_METHOD1(onVoteMessage, void(const VoteMessage &));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_GRANDPA_MOCK_HPP
