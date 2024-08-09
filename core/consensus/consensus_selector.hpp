/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/finality_consensus.hpp"
#include "consensus/production_consensus.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus {

  class ConsensusSelector {
   public:
    ConsensusSelector() = default;
    virtual ~ConsensusSelector() = default;

    ConsensusSelector(ConsensusSelector &&) = delete;
    ConsensusSelector(const ConsensusSelector &) = delete;
    ConsensusSelector &operator=(ConsensusSelector &&) = delete;
    ConsensusSelector &operator=(const ConsensusSelector &) = delete;

    virtual std::shared_ptr<ProductionConsensus> getProductionConsensus(
        const primitives::BlockInfo &parent_block) const = 0;

    virtual std::shared_ptr<ProductionConsensus> getProductionConsensus(
        const primitives::BlockHeader &block_header) const = 0;

    virtual std::shared_ptr<FinalityConsensus> getFinalityConsensus(
        const primitives::BlockInfo &parent_block) const = 0;

    virtual std::shared_ptr<FinalityConsensus> getFinalityConsensus(
        const primitives::BlockHeader &block_header) const = 0;
  };

}  // namespace kagome::consensus
