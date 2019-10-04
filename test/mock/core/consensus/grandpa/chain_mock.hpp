/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_CHAIN_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_CHAIN_MOCK_HPP

#include <gmock/gmock.h>
#include "consensus/grandpa/chain.hpp"

namespace kagome::consensus::grandpa {

  struct ChainMock : public Chain {
    ~ChainMock() override = default;

    MOCK_METHOD2(getAncestry,
                 outcome::result<std::vector<primitives::BlockHash>>(
                     primitives::BlockHash base, BlockHash block));

    MOCK_METHOD1(bestChainContaining,
                 boost::optional<BlockInfo>(BlockHash base));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_CHAIN_MOCK_HPP
