/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/consensus_selector.hpp"
#include "utils/lru.hpp"

namespace kagome::consensus {

  class ConsensusSelectorImpl final : public ConsensusSelector {
   public:
    ConsensusSelectorImpl(
        std::vector<std::shared_ptr<ProductionConsensus>>
            production_consensuses,
        std::vector<std::shared_ptr<FinalityConsensus>> finality_consensuses);

    std::shared_ptr<ProductionConsensus> getProductionConsensus(
        const primitives::BlockInfo &parent_block) override;

    std::shared_ptr<FinalityConsensus> getFinalityConsensus(
        const primitives::BlockInfo &parent_block) override;

   private:
    std::vector<std::shared_ptr<ProductionConsensus>> production_consensuses_;
    std::vector<std::shared_ptr<FinalityConsensus>> finality_consensuses_;

    Lru<primitives::BlockInfo, std::shared_ptr<ProductionConsensus>> pc_cache_{
        20};
    Lru<primitives::BlockInfo, std::shared_ptr<FinalityConsensus>> fc_cache_{
        20};
  };

}  // namespace kagome::consensus
