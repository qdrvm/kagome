/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_CHAIN_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_CHAIN_MOCK_HPP

#include <gmock/gmock.h>
#include "consensus/grandpa/chain.hpp"

namespace kagome::consensus::grandpa {

  struct ChainMock : public virtual Chain {
    ~ChainMock() override = default;

    MOCK_METHOD(outcome::result<std::vector<primitives::BlockHash>>,
                getAncestry,
                (const BlockHash &base, const BlockHash &block),
                (const, override));

    MOCK_METHOD(bool,
                hasAncestry,
                (const BlockHash &base, const BlockHash &block),
                (const, override));

    MOCK_METHOD(outcome::result<BlockInfo>,
                bestChainContaining,
                (const BlockHash &base),
                (const, override));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_CHAIN_MOCK_HPP
