/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/block_appender_base.hpp"

#include "blockchain/block_tree.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/consensus_selector.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/voting_round_error.hpp"
#include "consensus/timeline/slots_util.hpp"

namespace kagome::consensus {

  BlockAppenderBase::BlockAppenderBase(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<babe::BabeConfigRepository> babe_config_repo,
      const EpochTimings &timings,
      std::shared_ptr<grandpa::Environment> grandpa_environment,
      LazySPtr<SlotsUtil> slots_util,
      std::shared_ptr<crypto::Hasher> hasher,
      LazySPtr<ConsensusSelector> consensus_selector)
      : block_tree_{std::move(block_tree)},
        babe_config_repo_{std::move(babe_config_repo)},
        timings_(timings),
        grandpa_environment_{std::move(grandpa_environment)},
        slots_util_{slots_util},
        hasher_{std::move(hasher)},
        consensus_selector_(consensus_selector) {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(babe_config_repo_ != nullptr);
    BOOST_ASSERT(grandpa_environment_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
  }

  void BlockAppenderBase::applyJustifications(
      const primitives::BlockInfo &block_info,
      const std::optional<primitives::Justification> &opt_justification,
      ApplyJustificationCb &&callback) {
    // apply justification if any (must be done strictly after block is
    // added and its consensus digests are handled)
    if (opt_justification.has_value()) {
      SL_VERBOSE(
          logger_, "Apply justification received for block {}", block_info);

      grandpa_environment_->applyJustification(
          block_info,
          opt_justification.value(),
          [logger{logger_}, block_info, callback{std::move(callback)}](
              const outcome::result<void> &result) {
            if (result.has_error()) {
              SL_ERROR(logger,
                       "Error while applying justification of block {}: {}",
                       block_info,
                       result.error());
            }
            callback(result);
          });
    } else {
      callback(outcome::success());
    }
  }

  outcome::result<void> BlockAppenderBase::validateHeader(
      const primitives::Block &block) {
    auto consensus =
        consensus_selector_.get()->getProductionConsensus(block.header);
    BOOST_ASSERT_MSG(consensus, "Must be returned at least fallback consensus");
    return consensus->validateHeader(block.header);
  }

  outcome::result<BlockAppenderBase::SlotInfo> BlockAppenderBase::getSlotInfo(
      const primitives::BlockHeader &header) const {
    auto consensus = consensus_selector_.get()->getProductionConsensus(header);
    BOOST_ASSERT_MSG(consensus, "Must be returned at least fallback consensus");
    OUTCOME_TRY(slot_number, consensus->getSlot(header));
    auto start_time = slots_util_.get()->slotStartTime(slot_number);
    auto slot_duration = timings_.slot_duration;
    return outcome::success(SlotInfo{start_time, slot_duration});
  }

}  // namespace kagome::consensus
