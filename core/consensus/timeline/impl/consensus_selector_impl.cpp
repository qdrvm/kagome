/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/consensus_selector_impl.hpp"

#include "blockchain/block_header_repository.hpp"

namespace kagome::consensus {

  ConsensusSelectorImpl::ConsensusSelectorImpl(
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
      std::vector<std::shared_ptr<ProductionConsensus>> production_consensuses,
      std::vector<std::shared_ptr<FinalityConsensus>> finality_consensuses)
      : header_repo_(std::move(header_repo)),
        production_consensuses_(std::move(production_consensuses)),
        finality_consensuses_(std::move(finality_consensuses)) {
    BOOST_ASSERT(header_repo_ != nullptr);
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

    [[unlikely]] if (parent_block.number == 0) {
      for (size_t i = 0; i < production_consensuses_.size(); ++i) {
        const auto &consensus = production_consensuses_[i];
        if (consensus->isGenesisConsensus()) {
          return pc_cache_.put(parent_block, consensus);
        }
      }
    }

    auto header_res = header_repo_->getBlockHeader(parent_block.hash);
    if (header_res.has_error()) {
      // TODO Log it
      return {};
    }
    const auto &header = header_res.value();

    for (size_t i = 0; i < production_consensuses_.size(); ++i) {
      const auto &consensus = production_consensuses_[i];

      // Try to fit consensus by getting slot
      if (consensus->getSlot(header)) {
        return pc_cache_.put(parent_block, consensus);
      }

      // The last one is fallback
      if (i == production_consensuses_.size() - 1) {
        return pc_cache_.put(parent_block, consensus);
      }
    }
    BOOST_UNREACHABLE_RETURN({});
  }

  std::shared_ptr<ProductionConsensus>
  ConsensusSelectorImpl::getProductionConsensus(
      const primitives::BlockHeader &header) const {
    auto consensus_opt = pc_cache_.get(header.blockInfo());
    if (consensus_opt.has_value()) {
      return consensus_opt.value();
    }

    [[unlikely]] if (header.number == 0) {
      for (size_t i = 0; i < production_consensuses_.size(); ++i) {
        const auto &consensus = production_consensuses_[i];
        if (consensus->isGenesisConsensus()) {
          return consensus;
        }
      }
    }

    for (size_t i = 0; i < production_consensuses_.size(); ++i) {
      const auto &consensus = production_consensuses_[i];

      // Try to fit consensus by getting slot
      if (consensus->getSlot(header)) {
        return consensus;
      }

      // The last one is fallback
      if (i == production_consensuses_.size() - 1) {
        return consensus;
      }
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
      if (i == finality_consensuses_.size() - 1) {  // The last one is fallback
        return fc_cache_.put(parent_block, consensus);
      }

      // TODO: Code for trying to select another consensus
    }
    BOOST_UNREACHABLE_RETURN({});
  }

  std::shared_ptr<FinalityConsensus>
  ConsensusSelectorImpl::getFinalityConsensus(
      const primitives::BlockHeader &header) const {
    auto consensus_opt = fc_cache_.get(header.blockInfo());
    if (consensus_opt.has_value()) {
      return consensus_opt.value();
    }

    for (size_t i = 0; i < finality_consensuses_.size(); ++i) {
      const auto &consensus = finality_consensuses_[i];
      if (i == finality_consensuses_.size() - 1) {  // The last one is fallback
        return consensus;
      }

      // TODO: Code for trying to select another consensus
    }
    BOOST_UNREACHABLE_RETURN({});
  }

}  // namespace kagome::consensus
