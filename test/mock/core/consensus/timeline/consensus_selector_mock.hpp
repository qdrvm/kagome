/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/consensus_selector.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {

  class ConsensusSelectorMock final : public ConsensusSelector {
   public:
    MOCK_METHOD(std::shared_ptr<ProductionConsensus>,
                getProductionConsensus,
                (const primitives::BlockInfo &),
                (const, override));

    MOCK_METHOD(std::shared_ptr<FinalityConsensus>,
                getFinalityConsensus,
                (const primitives::BlockInfo &),
                (const, override));
  };

}  // namespace kagome::consensus
