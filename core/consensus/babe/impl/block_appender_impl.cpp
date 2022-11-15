/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/block_appender_impl.hpp"

#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "blockchain/digest_tracker.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/babe_util.hpp"
#include "consensus/babe/consistency_keeper.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/voting_round_error.hpp"
#include "consensus/validation/block_validator.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::babe,
                            BlockAppenderImpl::Error,
                            e) {
  using E = kagome::consensus::babe::BlockAppenderImpl::Error;
  switch (e) {
    case E::INVALID_BLOCK:
      return "Invalid block";
    case E::PARENT_NOT_FOUND:
      return "Parent not found";
  }
  return "Unknown error";
}

namespace kagome::consensus::babe {

  BlockAppenderImpl::BlockAppenderImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo,
      std::shared_ptr<BlockValidator> block_validator,
      std::shared_ptr<grandpa::Environment> grandpa_environment,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<blockchain::DigestTracker> digest_tracker,
      std::shared_ptr<BabeUtil> babe_util,
      std::shared_ptr<babe::ConsistencyKeeper> consistency_keeper)
      : block_tree_{std::move(block_tree)},
        babe_config_repo_{std::move(babe_config_repo)},
        block_validator_{std::move(block_validator)},
        grandpa_environment_{std::move(grandpa_environment)},
        hasher_{std::move(hasher)},
        digest_tracker_(std::move(digest_tracker)),
        babe_util_(std::move(babe_util)),
        consistency_keeper_(std::move(consistency_keeper)),
        logger_{log::createLogger("BlockAppender", "block_appender")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(babe_config_repo_ != nullptr);
    BOOST_ASSERT(block_validator_ != nullptr);
    BOOST_ASSERT(grandpa_environment_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(digest_tracker_ != nullptr);
    BOOST_ASSERT(babe_util_ != nullptr);
    BOOST_ASSERT(consistency_keeper_ != nullptr);
    BOOST_ASSERT(logger_ != nullptr);
  }

  outcome::result<void> BlockAppenderImpl::appendBlock(
      primitives::BlockData &&b) {
    if (not b.header.has_value()) {
      logger_->warn("Skipping a block without header");
      return Error::INVALID_BLOCK;
    }
    auto &header = b.header.value();

    auto block_hash = hasher_->blake2b_256(scale::encode(header).value());

    primitives::BlockInfo block_info(header.number, block_hash);

    if (last_appended_.has_value()) {
      if (last_appended_->number > block_info.number) {
        SL_TRACE(
            logger_, "Skip early appended header of block: {}", block_info);
        return outcome::success();
      }
      if (last_appended_.value() == block_info) {
        SL_TRACE(logger_, "Skip just appended header of block: {}", block_info);
        return outcome::success();
      }
    }

    if (last_appended_
        != primitives::BlockInfo(header.number - 1, header.parent_hash)) {
      if (auto header_res = block_tree_->getBlockHeader(header.parent_hash);
          header_res.has_error()
          && header_res.error()
                 == blockchain::BlockTreeError::HEADER_NOT_FOUND) {
        logger_->warn("Skipping a block {} with unknown parent", block_info);
        return Error::PARENT_NOT_FOUND;
      } else if (header_res.has_error()) {
        return header_res.as_failure();
      }
    }

    // get current time to measure performance if block execution
    auto t_start = std::chrono::high_resolution_clock::now();

    primitives::Block block{.header = std::move(header)};

    // check if block header already exists. If so, do not append
    if (auto header_res = block_tree_->getBlockHeader(block_hash);
        header_res.has_value()) {
      SL_DEBUG(logger_, "Skip existing header of block: {}", block_info);

      OUTCOME_TRY(block_tree_->addExistingBlock(block_hash, block.header));
    } else if (header_res.error()
               != blockchain::BlockTreeError::HEADER_NOT_FOUND) {
      return header_res.as_failure();
    } else {
      OUTCOME_TRY(block_tree_->addBlockHeader(block.header));
    }

    OUTCOME_TRY(babe_digests, getBabeDigests(block.header));

    const auto &babe_header = babe_digests.second;

    auto slot_number = babe_header.slot_number;

    babe_util_->syncEpoch([&] {
      auto res = block_tree_->getBlockHeader(primitives::BlockNumber(1));
      if (res.has_error()) {
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

      const auto &first_block_header = res.value();
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
        block_info,
        to_string(babe_header.slotType()),
        slot_number,
        epoch_number,
        babe_header.authority_index);

    auto consistency_guard = consistency_keeper_->start(block_info);

    // observe digest of block
    // (must be done strictly after block will be added)
    auto digest_tracking_res =
        digest_tracker_->onDigest(block_info, block.header.digest);
    if (digest_tracking_res.has_error()) {
      SL_ERROR(logger_,
               "Error while tracking digest of block {}: {}",
               block_info,
               digest_tracking_res.error());
      return digest_tracking_res.as_failure();
    }

    auto babe_config = babe_config_repo_->config(block_info, epoch_number);
    if (babe_config == nullptr) {
      return Error::INVALID_BLOCK;  // TODO Change to more appropriate error
    }

    SL_TRACE(logger_,
             "Actual epoch digest to apply block {} (slot {}, epoch {}). "
             "Randomness: {}",
             block_info,
             slot_number,
             epoch_number,
             babe_config->randomness);

    auto threshold = calculateThreshold(babe_config->leadership_rate,
                                        babe_config->authorities,
                                        babe_header.authority_index);

    OUTCOME_TRY(block_validator_->validateHeader(
        block.header,
        epoch_number,
        babe_config->authorities[babe_header.authority_index].id,
        threshold,
        *babe_config));

    // try to apply postponed justifications first if any
    if (not postponed_justifications_.empty()) {
      std::vector<primitives::BlockInfo> to_remove;
      for (const auto &[block_justified_for, justification] :
           postponed_justifications_) {
        SL_DEBUG(logger_,
                 "Try to apply postponed justification received for block {}",
                 block_justified_for);
        auto res = applyJustification(block_justified_for, justification);
        if (res.has_value()) {
          to_remove.push_back(block_justified_for);
        }
      }
      for (const auto &item : to_remove) {
        postponed_justifications_.erase(item);
      }
    }

    // apply justification if any (must be done strictly after block will be
    // added and his consensus-digests will be handled)
    if (b.justification.has_value()) {
      SL_VERBOSE(
          logger_, "Apply justification received for block {}", block_info);

      auto res = applyJustification(block_info, b.justification.value());
      if (res.has_error()) {
        if (res
            == outcome::failure(grandpa::VotingRoundError::NOT_ENOUGH_WEIGHT)) {
          postponed_justifications_.emplace(block_info,
                                            b.justification.value());
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
        // safely could be remove if current justification applied successfully
        postponed_justifications_.clear();
      }
    }

    auto now = std::chrono::high_resolution_clock::now();

    SL_DEBUG(
        logger_,
        "Imported header of block {} within {} us",
        block_info,
        std::chrono::duration_cast<std::chrono::microseconds>(now - t_start)
            .count());

    auto block_delta = block_info.number - speed_data_.block_number;
    auto time_delta = now - speed_data_.time;
    if (block_delta >= 10000 or time_delta >= std::chrono::minutes(1)) {
      SL_INFO(logger_,
              "Imported {} more headers of blocks. Average speed is {} bps",
              block_delta,
              block_delta
                  / std::chrono::duration_cast<std::chrono::seconds>(time_delta)
                        .count());
      speed_data_.block_number = block_info.number;
      speed_data_.time = now;
    }

    consistency_guard.commit();

    last_appended_.emplace(std::move(block_info));

    return outcome::success();
  }

  outcome::result<void> BlockAppenderImpl::applyJustification(
      const primitives::BlockInfo &block_info,
      const primitives::Justification &justification) {
    return grandpa_environment_->applyJustification(block_info, justification);
  }

}  // namespace kagome::consensus::babe
