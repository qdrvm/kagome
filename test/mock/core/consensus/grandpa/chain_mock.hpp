/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>
#include "consensus/grandpa/chain.hpp"

namespace kagome::consensus::grandpa {

  struct ChainMock : public virtual Chain {
    ~ChainMock() override = default;

    MOCK_METHOD(outcome::result<bool>,
                hasBlock,
                (const primitives::BlockHash &),
                (const, override));

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
                (const BlockHash &, std::optional<VoterSetId> set_id),
                (const, override));
  };

}  // namespace kagome::consensus::grandpa
