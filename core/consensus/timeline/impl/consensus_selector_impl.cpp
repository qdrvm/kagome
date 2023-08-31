/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/consensus_selector_impl.hpp"

namespace kagome::consensus {

  ConsensusSelectorImpl::ConsensusSelectorImpl(
      std::vector<std::shared_ptr<ProductionConsensus>> production_consensuses,
      std::vector<std::shared_ptr<FinalityConsensus>> finality_consensuses)
      : production_consensuses_(std::move(production_consensuses)),
        finality_consensuses_(std::move(finality_consensuses)) {
    BOOST_ASSERT(not production_consensuses_.empty());
    BOOST_ASSERT(std::all_of(
        production_consensuses_.begin(),
        production_consensuses_.end(),
        [](const auto &consensus) { return consensus != nullptr; }));
    BOOST_ASSERT(not finality_consensuses_.empty());
    BOOST_ASSERT(std::all_of(
        finality_consensuses_.begin(),
        finality_consensuses_.end(),
        [](const auto &consensus) { return consensus != nullptr; }));
  }

  std::shared_ptr<ProductionConsensus>
  ConsensusSelectorImpl::getProductionConsensus(
      const primitives::BlockInfo &parent_block) const {
    auto consensus_opt = pc_cache_.get(parent_block);
    if (consensus_opt.has_value()) {
      return consensus_opt.value();
    }

    for (size_t i = 0; i < production_consensuses_.size(); ++i) {
      const auto &consensus = production_consensuses_[i];
      if (i == production_consensuses_.size() - 1) {  // Last one is fallback
        return pc_cache_.put(parent_block, consensus);
      }

      // TODO: Code for trying to select another consensus
    }
    BOOST_UNREACHABLE_RETURN({});
  }

  std::shared_ptr<FinalityConsensus>
  ConsensusSelectorImpl::getFinalityConsensus(
      const primitives::BlockInfo &parent_block) const {
    auto consensus_opt = fc_cache_.get(parent_block);
    if (consensus_opt.has_value()) {
      return consensus_opt.value();
    }

    for (size_t i = 0; i < finality_consensuses_.size(); ++i) {
      const auto &consensus = finality_consensuses_[i];
      if (i == finality_consensuses_.size() - 1) {  // Last one is fallback
        return fc_cache_.put(parent_block, consensus);
      }

      // TODO: Code for trying to select another consensus
    }
    BOOST_UNREACHABLE_RETURN({});
  };

}  // namespace kagome::consensus
