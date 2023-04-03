/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/block_appender_base.hpp"

#include "blockchain/block_tree.hpp"
#include "blockchain/digest_tracker.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/babe_error.hpp"
#include "consensus/babe/babe_util.hpp"
#include "consensus/babe/consistency_keeper.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/voting_round_error.hpp"
#include "consensus/validation/block_validator.hpp"

namespace kagome::consensus::babe {

  BlockAppenderBase::BlockAppenderBase(
      std::shared_ptr<ConsistencyKeeper> consistency_keeper,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::DigestTracker> digest_tracker,
      std::shared_ptr<BabeConfigRepository> babe_config_repo,
      std::shared_ptr<BlockValidator> block_validator,
      std::shared_ptr<grandpa::Environment> grandpa_environment,
      std::shared_ptr<BabeUtil> babe_util,
      std::shared_ptr<crypto::Hasher> hasher)
      : consistency_keeper_{std::move(consistency_keeper)},
        block_tree_{std::move(block_tree)},
        digest_tracker_{std::move(digest_tracker)},
        babe_config_repo_{std::move(babe_config_repo)},
        block_validator_{std::move(block_validator)},
        grandpa_environment_{std::move(grandpa_environment)},
        babe_util_{std::move(babe_util)},
        hasher_{std::move(hasher)} {
    BOOST_ASSERT(nullptr != consistency_keeper_);
    BOOST_ASSERT(nullptr != block_tree_);
    BOOST_ASSERT(nullptr != digest_tracker_);
    BOOST_ASSERT(nullptr != babe_config_repo_);
    BOOST_ASSERT(nullptr != block_validator_);
    BOOST_ASSERT(nullptr != grandpa_environment_);
    BOOST_ASSERT(nullptr != babe_util_);
    BOOST_ASSERT(nullptr != hasher_);
  }

  primitives::BlockContext BlockAppenderBase::makeBlockContext(
      primitives::BlockHeader const &header) const {
    auto block_hash = hasher_->blake2b_256(scale::encode(header).value());
    return primitives::BlockContext{
        .block_info = {header.number, block_hash},
        .header = header,
    };
  }

  outcome::result<void> BlockAppenderBase::applyJustifications(
      const primitives::BlockInfo &block_info,
      std::optional<primitives::Justification> const &opt_justification) {
    // try to apply postponed justifications first if any
    if (not postponed_justifications_.empty()) {
      std::vector<primitives::BlockInfo> to_remove;
      for (const auto &[block_justified_for, justification] :
           postponed_justifications_) {
        SL_DEBUG(logger_,
                 "Try to apply postponed justification received for block {}",
                 block_justified_for);
        auto res = grandpa_environment_->applyJustification(block_justified_for,
                                                            justification);
        if (res.has_value()) {
          to_remove.push_back(block_justified_for);
        }
      }
      for (const auto &item : to_remove) {
        postponed_justifications_.erase(item);
      }
    }

    // apply justification if any (must be done strictly after block is
    // added and its consensus digests are handled)
    if (opt_justification.has_value()) {
      SL_VERBOSE(
          logger_, "Apply justification received for block {}", block_info);

      auto res = grandpa_environment_->applyJustification(
          block_info, opt_justification.value());
      if (res.has_error()) {
        // If the total weight is not enough, this justification is deferred to
        // try to apply it after the next block is added. One of the reasons for
        // this error is the presence of preliminary votes for future blocks
        // that have not yet been applied.
        if (res
            == outcome::failure(grandpa::VotingRoundError::NOT_ENOUGH_WEIGHT)) {
          postponed_justifications_.emplace(block_info,
                                            opt_justification.value());
          SL_VERBOSE(logger_,
                     "Postpone justification received for block {}: {}",
                     block_info,
                     res);
        } else {
          SL_ERROR(logger_,
                   "Error while applying justification of block {}: {}",
                   block_info,
                   res.error());
          return res.as_failure();
        }
      } else {
        // safely could be cleared if current justification applied successfully
        postponed_justifications_.clear();
      }
    }
    return outcome::success();
  }

  outcome::result<ConsistencyGuard>
  BlockAppenderBase::observeDigestsAndValidateHeader(
      primitives::Block const &block, primitives::BlockContext const &context) {
    OUTCOME_TRY(babe_digests, getBabeDigests(block.header));

    const auto &babe_header = babe_digests.second;

    auto slot_number = babe_header.slot_number;

    babe_util_->syncEpoch([&] {
      auto hash_res = block_tree_->getBlockHash(primitives::BlockNumber(1));
      if (hash_res.has_error() || !hash_res.value().has_value()) {
        if (block.header.number == 1) {
          SL_TRACE(logger_,
                   "First block slot is {}: it is first block (at executing)",
                   slot_number);
          return std::tuple(slot_number, false);
        } else {
          SL_TRACE(logger_,
                   "First block slot is {}: no first block (at executing)",
                   babe_util_->getCurrentSlot());
          return std::tuple(babe_util_->getCurrentSlot(), false);
        }
      }

      auto first_block_header_res =
          block_tree_->getBlockHeader(*hash_res.value());
      if (first_block_header_res.has_error()) {
        SL_CRITICAL(logger_,
                    "Database is not consistent: "
                    "Not found block header for existing num-to-hash record");
        throw std::runtime_error(
            "Not found block header for existing num-to-hash record");
      }

      const auto &first_block_header = first_block_header_res.value();
      auto babe_digest_res = getBabeDigests(first_block_header);
      BOOST_ASSERT_MSG(babe_digest_res.has_value(),
                       "Any non genesis block must contain babe digest");
      auto first_slot_number = babe_digest_res.value().second.slot_number;

      auto is_first_block_finalized =
          block_tree_->getLastFinalized().number > 0;

      SL_TRACE(
          logger_,
          "First block slot is {}: by {}finalized first block (at executing)",
          first_slot_number,
          is_first_block_finalized ? "" : "non-");
      return std::tuple(first_slot_number, is_first_block_finalized);
    });

    auto epoch_number = babe_util_->slotToEpoch(slot_number);

    SL_VERBOSE(
        logger_,
        "Appending header of block {} ({} in slot {}, epoch {}, authority #{})",
        context.block_info,
        to_string(babe_header.slotType()),
        slot_number,
        epoch_number,
        babe_header.authority_index);

    auto consistency_guard = consistency_keeper_->start(context.block_info);

    auto digest_tracking_res =
        digest_tracker_->onDigest(context, block.header.digest);
    if (digest_tracking_res.has_error()) {
      SL_ERROR(logger_,
               "Error while tracking digest of block {}: {}",
               context.block_info,
               digest_tracking_res.error());
      return digest_tracking_res.as_failure();
    }

    auto babe_config_opt = babe_config_repo_->config(context, epoch_number);
    if (!babe_config_opt.has_value()) {
      return BlockAdditionError::ORPHAN_BLOCK;
    }
    auto &babe_config = babe_config_opt.value().get();

    SL_TRACE(logger_,
             "Actual epoch digest to apply block {} (slot {}, epoch {}). "
             "Randomness: {}",
             context.block_info,
             slot_number,
             epoch_number,
             babe_config.randomness);

    auto threshold = calculateThreshold(babe_config.leadership_rate,
                                        babe_config.authorities,
                                        babe_header.authority_index);

    OUTCOME_TRY(block_validator_->validateHeader(
        block.header,
        epoch_number,
        babe_config.authorities[babe_header.authority_index].id,
        threshold,
        babe_config));

    return consistency_guard;
  }

  outcome::result<BlockAppenderBase::SlotInfo> BlockAppenderBase::getSlotInfo(
      primitives::BlockHeader const &header) const {
    OUTCOME_TRY(babe_digests, getBabeDigests(header));

    const auto &babe_header = babe_digests.second;

    auto slot_number = babe_header.slot_number;

    auto start_time = babe_util_->slotStartTime(slot_number);
    auto slot_duration = babe_config_repo_->slotDuration();
    return outcome::success(SlotInfo{start_time, slot_duration});
  }

}  // namespace kagome::consensus::babe
