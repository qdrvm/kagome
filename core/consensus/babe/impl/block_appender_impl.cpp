/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/block_appender_impl.hpp"

#include <chrono>

#include "blockchain/block_tree_error.hpp"
#include "blockchain/impl/common.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "consensus/babe/types/slot.hpp"
#include "consensus/grandpa/impl/voting_round_error.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "primitives/common.hpp"
#include "scale/scale.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

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

namespace {
  constexpr const char *kBlockExecutionTime =
      "kagome_block_verification_and_import_time";
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
      std::shared_ptr<BabeUtil> babe_util)
      : block_tree_{std::move(block_tree)},
        babe_configuration_{std::move(configuration)},
        block_validator_{std::move(block_validator)},
        grandpa_environment_{std::move(grandpa_environment)},
        hasher_{std::move(hasher)},
        authority_update_observer_{std::move(authority_update_observer)},
        babe_util_(std::move(babe_util)),
        logger_{log::createLogger("BlockAppender", "block_appender")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(babe_configuration_ != nullptr);
    BOOST_ASSERT(block_validator_ != nullptr);
    BOOST_ASSERT(grandpa_environment_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(authority_update_observer_ != nullptr);
    BOOST_ASSERT(babe_util_ != nullptr);
    BOOST_ASSERT(logger_ != nullptr);
  }

  outcome::result<void> BlockAppenderImpl::appendBlock(
      primitives::BlockData &&b) {
    if (not b.header.has_value()) {
      logger_->warn("Skipping a block without header");
      return Error::INVALID_BLOCK;
    }
    auto &header = b.header.value();

    if (auto header_res = block_tree_->getBlockHeader(header.parent_hash);
        header_res.has_error()
        && header_res.error() == blockchain::BlockTreeError::HEADER_NOT_FOUND) {
      logger_->warn("Skipping a block with unknown parent");
      return Error::PARENT_NOT_FOUND;
    } else if (header_res.has_error()) {
      return header_res.as_failure();
    }

    // get current time to measure performance if block execution
    auto t_start = std::chrono::high_resolution_clock::now();

    auto block_hash = hasher_->blake2b_256(scale::encode(header).value());

    bool block_already_exists = false;

    // check if block header already exists. If so, do not append
    if (auto header_res = block_tree_->getBlockHeader(block_hash);
        header_res.has_value()) {
      SL_DEBUG(logger_,
               "Skip existing block: {}",
               primitives::BlockInfo(header.number, block_hash));

      OUTCOME_TRY(block_tree_->addExistingBlock(block_hash, header));
      block_already_exists = true;
    } else if (header_res.error()
               != blockchain::BlockTreeError::HEADER_NOT_FOUND) {
      return header_res.as_failure();
    }

    primitives::Block block{.header = std::move(header)};

    OUTCOME_TRY(babe_digests, getBabeDigests(block.header));

    const auto &babe_header = babe_digests.second;

    auto slot_number = babe_header.slot_number;
    auto epoch_number = babe_util_->slotToEpoch(slot_number);

    logger_->info(
        "Applying block {} ({} in slot {}, epoch {})",  //
        primitives::BlockInfo(block.header.number, block_hash),
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
             primitives::BlockInfo(block.header.number, block_hash),
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
                 slot_number,
                 primitives::BlockInfo(block.header.number, block_hash),
                 next_epoch_digest.randomness);
    }

    auto block_without_seal_digest = block;

    // block should be applied without last digest which contains the seal
    block_without_seal_digest.header.digest.pop_back();

    auto parent = block_tree_->getBlockHeader(block.header.parent_hash).value();

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
            return authority_update_observer_->onConsensus(
                primitives::BlockInfo{block.header.number, block_hash},
                consensus_message);
          },
          [](const auto &) { return outcome::success(); });
      if (res.has_error()) {
        rollbackBlock(block_hash);
        return res.as_failure();
      }
    }

    // apply justification if any (must be done strictly after block will be
    // added and his consensus-digests will be handled)
    if (b.justification.has_value()) {
      SL_VERBOSE(logger_,
                 "Justification received for block {}",
                 primitives::BlockInfo(block.header.number, block_hash));
      if (not justifications_.empty()) {
        std::vector<primitives::BlockInfo> to_remove;
        for (const auto &[block_info, justification] : justifications_) {
          auto res = grandpa_environment_->applyJustification(block_info,
                                                              justification);
          if (res) {
            to_remove.push_back(block_info);
          }
        }
        if (not to_remove.empty()) {
          for (const auto &item : to_remove) {
            justifications_.erase(item);
          }
        }
      }
      auto res = grandpa_environment_->applyJustification(
          primitives::BlockInfo(block.header.number, block_hash),
          b.justification.value());
      if (res.has_error()) {
        if (res.error() == grandpa::VotingRoundError::NOT_ENOUGH_WEIGHT) {
          justifications_.emplace(
              primitives::BlockInfo(block.header.number, block_hash),
              b.justification.value());
        } else {
          rollbackBlock(block_hash);
          return res.as_failure();
        }
      }
    }

    auto t_end = std::chrono::high_resolution_clock::now();

    logger_->info(
        "Imported block {} within {} ms",
        primitives::BlockInfo(block.header.number, block_hash),
        std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start)
            .count());

    return outcome::success();
  }

  void BlockAppenderImpl::rollbackBlock(
      const primitives::BlockHash &block_hash) {
    auto removal_res = block_tree_->removeLeaf(block_hash);
    if (removal_res.has_error()) {
      SL_WARN(logger_,
              "Rolling back of block {} is failed: {}",
              block_hash,
              removal_res.error().message());
    }
  }

}  // namespace kagome::consensus
