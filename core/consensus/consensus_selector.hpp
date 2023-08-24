/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_CONSENSUSSELECTOR
#define KAGOME_CONSENSUS_CONSENSUSSELECTOR

#include "consensus/finality_consensus.hpp"
#include "consensus/production_consensus.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus {

  class ConsensusSelector {
   public:
    ConsensusSelector() = default;
    virtual ~ConsensusSelector() = default;

    ConsensusSelector(ConsensusSelector &&) noexcept = delete;
    ConsensusSelector(const ConsensusSelector &) = delete;
    ConsensusSelector &operator=(ConsensusSelector &&) noexcept = delete;
    ConsensusSelector &operator=(const ConsensusSelector &) = delete;

    virtual std::shared_ptr<ProductionConsensus> getProductionConsensus(
        const primitives::BlockInfo &parent_block) = 0;

    virtual std::shared_ptr<FinalityConsensus> getFinalityConsensus(
        const primitives::BlockInfo &parent_block) = 0;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_CONSENSUSSELECTOR
