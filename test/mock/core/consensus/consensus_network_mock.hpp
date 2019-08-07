/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_NETWORK_MOCK_HPP
#define KAGOME_CONSENSUS_NETWORK_MOCK_HPP

#include "consensus/consensus_network.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {
  struct ConsensusNetworkMock : public ConsensusNetwork {
    MOCK_METHOD1(broadcast, void(const primitives::Block &));
  };
}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_NETWORK_MOCK_HPP
