/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/environment.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"

namespace kagome::blockchain {
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::consensus {
  class SlotsUtil;
  class ConsensusSelector;
}  // namespace kagome::consensus

namespace kagome::consensus::babe {
  class BabeConfigRepository;
}

namespace kagome::consensus {

  /**
   * Common logic for adding a new block to the blockchain
   */
  class BlockAppenderBase {
   public:
    using ApplyJustificationCb = std::function<void(outcome::result<void>)>;
    BlockAppenderBase(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<babe::BabeConfigRepository> babe_config_repo,
        const EpochTimings &timings,
        std::shared_ptr<grandpa::Environment> grandpa_environment,
        LazySPtr<SlotsUtil> slots_util,
        std::shared_ptr<crypto::Hasher> hasher,
        LazySPtr<ConsensusSelector> consensus_selector);

    void applyJustifications(
        const primitives::BlockInfo &block_info,
        const std::optional<primitives::Justification> &new_justification,
        ApplyJustificationCb &&callback);

    outcome::result<void> validateHeader(const primitives::Block &block);

    struct SlotInfo {
      TimePoint start;
      Duration duration;
    };

    outcome::result<SlotInfo> getSlotInfo(
        const primitives::BlockHeader &header) const;

   private:
    log::Logger logger_ = log::createLogger("BlockAppender", "babe");

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<babe::BabeConfigRepository> babe_config_repo_;
    const EpochTimings &timings_;
    std::shared_ptr<grandpa::Environment> grandpa_environment_;
    LazySPtr<SlotsUtil> slots_util_;
    std::shared_ptr<crypto::Hasher> hasher_;
    LazySPtr<ConsensusSelector> consensus_selector_;
  };

}  // namespace kagome::consensus
