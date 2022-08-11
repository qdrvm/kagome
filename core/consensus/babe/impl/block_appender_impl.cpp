/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/block_appender_impl.hpp"

#include <chrono>

#include "blockchain/block_tree_error.hpp"
#include "consensus/babe/consistency_keeper.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "consensus/babe/types/slot.hpp"
#include "consensus/grandpa/impl/voting_round_error.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "primitives/common.hpp"
#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, BlockAppenderImpl::Error, e) {
  using E = kagome::consensus::BlockAppenderImpl::Error;
  switch (e) {
    case E::INVALID_BLOCK:
      return "Invalid block";
    case E::PARENT_NOT_FOUND:
      return "Parent not found";
  }
  return "Unknown error";
}

namespace kagome::consensus {

  BlockAppenderImpl::BlockAppenderImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<primitives::BabeConfiguration> configuration,
      std::shared_ptr<BlockValidator> block_validator,
      std::shared_ptr<grandpa::Environment> grandpa_environment,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<authority::AuthorityUpdateObserver>
          authority_update_observer,
      std::shared_ptr<BabeUtil> babe_util,
      std::shared_ptr<babe::ConsistencyKeeper> consistency_keeper)
      : block_tree_{std::move(block_tree)},
        babe_configuration_{std::move(configuration)},
        block_validator_{std::move(block_validator)},
        grandpa_environment_{std::move(grandpa_environment)},
        hasher_{std::move(hasher)},
        authority_update_observer_{std::move(authority_update_observer)},
        babe_util_(std::move(babe_util)),
        consistency_keeper_(std::move(consistency_keeper)),
        logger_{log::createLogger("BlockAppender", "block_appender")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(babe_configuration_ != nullptr);
    BOOST_ASSERT(block_validator_ != nullptr);
    BOOST_ASSERT(grandpa_environment_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(authority_update_observer_ != nullptr);
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
        SL_DEBUG(
            logger_, "Skip early appended header of block: {}", block_info);
        return outcome::success();
      }
      if (last_appended_.value() == block_info) {
        SL_DEBUG(logger_, "Skip just appended header of block: {}", block_info);
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

    bool block_already_exists = false;

    // check if block header already exists. If so, do not append
    if (auto header_res = block_tree_->getBlockHeader(block_hash);
        header_res.has_value()) {
      SL_DEBUG(logger_, "Skip existing header of block: {}", block_info);

      OUTCOME_TRY(block_tree_->addExistingBlock(block_hash, header));
      block_already_exists = true;
    } else if (header_res.error()
               != blockchain::BlockTreeError::HEADER_NOT_FOUND) {
      return header_res.as_failure();
    }

    auto consistency_guard = consistency_keeper_->start(block_info);

    primitives::Block block{.header = std::move(header)};

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
      auto babe_digest_res = consensus::getBabeDigests(first_block_header);
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

    SL_DEBUG(
        logger_,
        "Applying block {} ({} in slot {}, epoch {})",  //
        block_info,
        babe_header.slotType() == SlotType::Primary          ? "primary"
        : babe_header.slotType() == SlotType::SecondaryVRF   ? "secondary-vrf"
        : babe_header.slotType() == SlotType::SecondaryPlain ? "secondary-plain"
                                                             : "unknown",
        slot_number,
        epoch_number);

    OUTCOME_TRY(
        this_block_epoch_descriptor,
        block_tree_->getEpochDigest(epoch_number, block.header.parent_hash));

    SL_TRACE(logger_,
             "Actual epoch digest to apply block {} (slot {}, epoch {}). "
             "Randomness: {}",
             block_info,
             slot_number,
             epoch_number,
             this_block_epoch_descriptor.randomness);

    auto threshold = calculateThreshold(babe_configuration_->leadership_rate,
                                        this_block_epoch_descriptor.authorities,
                                        babe_header.authority_index);

    OUTCOME_TRY(block_validator_->validateHeader(
        block.header,
        epoch_number,
        this_block_epoch_descriptor.authorities[babe_header.authority_index].id,
        threshold,
        this_block_epoch_descriptor.randomness));

    if (auto next_epoch_digest_res = getNextEpochDigest(block.header)) {
      auto &next_epoch_digest = next_epoch_digest_res.value();
      SL_VERBOSE(logger_,
                 "Next epoch digest has gotten in block {} (slot {}). "
                 "Randomness: {}",
                 block_info,
                 slot_number,
                 next_epoch_digest.randomness);
    }

    auto block_without_seal_digest = block;

    // block should be applied without last digest which contains the seal
    block_without_seal_digest.header.digest.pop_back();

    if (not block_already_exists) {
      OUTCOME_TRY(block_tree_->addBlockHeader(block.header));
    }

    // observe possible changes of authorities
    // (must be done strictly after block will be added)
    for (auto &digest_item : block_without_seal_digest.header.digest) {
      auto res = visit_in_place(
          digest_item,
          [&](const primitives::Consensus &consensus_message)
              -> outcome::result<void> {
            return authority_update_observer_->onConsensus(block_info,
                                                           consensus_message);
          },
          [](const auto &) { return outcome::success(); });
      if (res.has_error()) {
        SL_ERROR(logger_,
                 "Error while processing consensus digests of block {}: {}",
                 block_info,
                 res.error().message());
        return res.as_failure();
      }
    }

    // apply justification if any (must be done strictly after block will be
    // added and his consensus-digests will be handled)
    if (b.justification.has_value()) {
      SL_VERBOSE(logger_, "Justification received for block {}", block_info);

      // try to apply left in justification store values first
      if (not justifications_.empty()) {
        std::vector<primitives::BlockInfo> to_remove;
        for (const auto &[block_justified_for, justification] :
             justifications_) {
          auto res = applyJustification(block_justified_for, justification);
          if (res) {
            to_remove.push_back(block_justified_for);
          }
        }
        if (not to_remove.empty()) {
          for (const auto &item : to_remove) {
            justifications_.erase(item);
          }
        }
      }

      auto res = applyJustification(block_info, b.justification.value());
      if (res.has_error()) {
        if (res
            == outcome::failure(grandpa::VotingRoundError::NOT_ENOUGH_WEIGHT)) {
          justifications_.emplace(block_info, b.justification.value());
        } else {
          SL_ERROR(logger_,
                   "Error while applying of block {} justification: {}",
                   block_info,
                   res.error().message());
          return res.as_failure();
        }
      } else {
        // safely could be remove if current justification applied successfully
        justifications_.clear();
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
    if (block_delta >= 20000 or time_delta >= std::chrono::minutes(1)) {
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

}  // namespace kagome::consensus
