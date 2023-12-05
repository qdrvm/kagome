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
        slots_util_{std::move(slots_util)},
        hasher_{std::move(hasher)},
        consensus_selector_(std::move(consensus_selector)) {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(babe_config_repo_ != nullptr);
    BOOST_ASSERT(grandpa_environment_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);

    postponed_justifications_ = std::make_shared<
        std::map<primitives::BlockInfo, primitives::Justification>>();
  }

  void BlockAppenderBase::applyJustifications(
      const primitives::BlockInfo &block_info,
      const std::optional<primitives::Justification> &opt_justification,
      ApplyJustificationCb &&callback) {
    // try to apply postponed justifications first if any
    if (not postponed_justifications_->empty()) {
      for (auto it = postponed_justifications_->begin();
           it != postponed_justifications_->end();) {
        const auto &block_justified_for = it->first;
        const auto &justification = it->second;

        SL_DEBUG(logger_,
                 "Try to apply postponed justification received for block {}",
                 block_justified_for);
        grandpa_environment_->applyJustification(
            block_justified_for,
            justification,
            [block_justified_for,
             wpp{std::weak_ptr<PostponedJustifications>{
                 postponed_justifications_}}](auto &&result) mutable {
              if (auto pp = wpp.lock()) {
                if (result.has_value()) {
                  pp->erase(block_justified_for);
                }
              }
            });
      }
    }

    // apply justification if any (must be done strictly after block is
    // added and its consensus digests are handled)
    if (opt_justification.has_value()) {
      SL_VERBOSE(
          logger_, "Apply justification received for block {}", block_info);

      grandpa_environment_->applyJustification(
          block_info,
          opt_justification.value(),
          [wlogger{log::WLogger{logger_}},
           callback{std::move(callback)},
           block_info,
           justification{opt_justification.value()},
           wpp{std::weak_ptr<PostponedJustifications>{
               postponed_justifications_}}](auto &&result) mutable {
            if (auto pp = wpp.lock()) {
              if (result.has_error()) {
                // If the total weight is not enough, this justification is
                // deferred to try to apply it after the next block is added.
                // One of the reasons for this error is the presence of
                // preliminary votes for future blocks that have not yet been
                // applied.
                if (result
                    == outcome::failure(
                        grandpa::VotingRoundError::NOT_ENOUGH_WEIGHT)) {
                  pp->emplace(block_info, justification);
                  if (auto logger = wlogger.lock()) {
                    SL_VERBOSE(
                        logger,
                        "Postpone justification received for block {}: {}",
                        block_info,
                        result);
                  }
                } else {
                  if (auto logger = wlogger.lock()) {
                    SL_ERROR(
                        logger,
                        "Error while applying justification of block {}: {}",
                        block_info,
                        result.error());
                  }
                  callback(result.as_failure());
                  return;
                }
              } else {
                // safely could be cleared if current justification applied
                // successfully
                pp->clear();
              }
              callback(outcome::success());
            }
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
    OUTCOME_TRY(
        slot_number,
        babe::getSlot(header));  // TODO(xDimon): Make it consensus agnostic
    auto start_time = slots_util_.get()->slotStartTime(slot_number);
    auto slot_duration = timings_.slot_duration;
    return outcome::success(SlotInfo{start_time, slot_duration});
  }

}  // namespace kagome::consensus
