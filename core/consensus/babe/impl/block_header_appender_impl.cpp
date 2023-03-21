/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/block_header_appender_impl.hpp"

#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "blockchain/digest_tracker.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/babe_error.hpp"
#include "consensus/babe/babe_util.hpp"
#include "consensus/babe/consistency_keeper.hpp"
#include "consensus/babe/impl/block_appender_base.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/validation/block_validator.hpp"

namespace kagome::consensus::babe {

  BlockHeaderAppenderImpl::BlockHeaderAppenderImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<crypto::Hasher> hasher,
      std::unique_ptr<BlockAppenderBase> appender)
      : block_tree_{std::move(block_tree)},
        hasher_{std::move(hasher)},
        appender_{std::move(appender)},
        logger_{log::createLogger("BlockHeaderAppender", "block_appender")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(logger_ != nullptr);
    BOOST_ASSERT(appender_ != nullptr);
  }

  outcome::result<void> BlockHeaderAppenderImpl::appendHeader(
      primitives::BlockHeader &&block_header,
      std::optional<primitives::Justification> const &justification) {
    auto block_context = appender_->makeBlockContext(block_header);
    auto &block_info = block_context.block_info;

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
        != primitives::BlockInfo(block_header.number - 1,
                                 block_header.parent_hash)) {
      if (auto header_res =
              block_tree_->getBlockHeader(block_header.parent_hash);
          header_res.has_error()
          && header_res.error()
                 == blockchain::BlockTreeError::HEADER_NOT_FOUND) {
        logger_->warn("Skipping a block {} with unknown parent", block_info);
        return BlockAdditionError::PARENT_NOT_FOUND;
      } else if (header_res.has_error()) {
        return header_res.as_failure();
      }
    }

    // get current time to measure performance if block execution
    auto t_start = std::chrono::high_resolution_clock::now();

    primitives::Block block{.header = std::move(block_header)};

    // check if block header already exists. If so, do not append
    if (auto header_res = block_tree_->getBlockHeader(block_info.hash);
        header_res.has_value()) {
      SL_DEBUG(logger_, "Skip existing header of block: {}", block_info);

      OUTCOME_TRY(block_tree_->addExistingBlock(block_info.hash, block.header));
    } else if (header_res.error()
               != blockchain::BlockTreeError::HEADER_NOT_FOUND) {
      return header_res.as_failure();
    } else {
      OUTCOME_TRY(block_tree_->addBlockHeader(block.header));
    }

    OUTCOME_TRY(
        consistency_guard,
        appender_->observeDigestsAndValidateHeader(block, block_context));

    OUTCOME_TRY(appender_->applyJustifications(block_info, justification));

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
      SL_LOG(
          logger_,
          speed_data_.block_number ? log::Level::INFO
                                   : static_cast<log::Level>(-1),
          "Imported {} more headers of blocks {}-{}. Average speed is {} bps",
          block_delta,
          speed_data_.block_number,
          block_info.number,
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

}  // namespace kagome::consensus::babe
