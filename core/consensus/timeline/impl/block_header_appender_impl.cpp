/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/block_header_appender_impl.hpp"

#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "blockchain/digest_tracker.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/timeline/consistency_keeper.hpp"
#include "consensus/timeline/impl/block_addition_error.hpp"
#include "consensus/timeline/impl/block_appender_base.hpp"
#include "consensus/validation/block_validator.hpp"

namespace kagome::consensus {

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

  void BlockHeaderAppenderImpl::appendHeader(
      primitives::BlockHeader &&block_header,
      const std::optional<primitives::Justification> &justification,
      ApplyJustificationCb &&callback) {
    auto block_context = appender_->makeBlockContext(block_header);
    auto &block_info = block_context.block_info;

    if (last_appended_.has_value()) {
      if (last_appended_->number > block_info.number) {
        SL_TRACE(
            logger_, "Skip early appended header of block: {}", block_info);
        callback(outcome::success());
        return;
      }
      if (last_appended_.value() == block_info) {
        SL_TRACE(logger_, "Skip just appended header of block: {}", block_info);
        callback(outcome::success());
        return;
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
        callback(BlockAdditionError::PARENT_NOT_FOUND);
        return;
      } else if (header_res.has_error()) {
        callback(header_res.as_failure());
        return;
      }
    }

    // get current time to measure performance if block execution
    const auto t_start = std::chrono::high_resolution_clock::now();

    primitives::Block block{.header = std::move(block_header)};

    // check if block header already exists. If so, do not append
    if (auto header_res = block_tree_->getBlockHeader(block_info.hash);
        header_res.has_value()) {
      SL_DEBUG(logger_, "Skip existing header of block: {}", block_info);

      if (auto res =
              block_tree_->addExistingBlock(block_info.hash, block.header);
          res.has_error()) {
        callback(res.as_failure());
        return;
      }
    } else if (header_res.error()
               != blockchain::BlockTreeError::HEADER_NOT_FOUND) {
      callback(header_res.as_failure());
      return;
    } else {
      if (auto res = block_tree_->addBlockHeader(block.header);
          res.has_error()) {
        callback(res.as_failure());
        return;
      }
    }

    std::optional<ConsistencyGuard> consistency_guard{};
    if (auto res =
            appender_->observeDigestsAndValidateHeader(block, block_context);
        res.has_value()) {
      consistency_guard.emplace(std::move(res.value()));
    } else {
      callback(res.as_failure());
      return;
    }

    BOOST_ASSERT(consistency_guard);
    consistency_guard->commit();

    appender_->applyJustifications(
        block_info,
        justification,
        [wself{weak_from_this()},
         block_info,
         t_start,
         callback{std::move(callback)}](auto &&result) mutable {
          auto self = wself.lock();
          if (!self) {
            callback(BlockAdditionError::NO_INSTANCE);
            return;
          }
          auto const now = std::chrono::high_resolution_clock::now();

          SL_DEBUG(self->logger_,
                   "Imported header of block {} within {} us",
                   block_info,
                   std::chrono::duration_cast<std::chrono::microseconds>(
                       now - t_start)
                       .count());

          auto const block_delta =
              block_info.number - self->speed_data_.block_number;
          auto const time_delta = now - self->speed_data_.time;
          if (block_delta >= 10000 or time_delta >= std::chrono::minutes(1)) {
            const auto td =
                std::chrono::duration_cast<std::chrono::seconds>(time_delta)
                    .count();
            SL_LOG(self->logger_,
                   self->speed_data_.block_number ? log::Level::INFO
                                                  : static_cast<log::Level>(-1),
                   "Imported {} more headers of blocks {}-{}. Average speed is "
                   "{} bps",
                   block_delta,
                   self->speed_data_.block_number,
                   block_info.number,
                   td != 0ull ? block_delta / td : 0ull);
            self->speed_data_.block_number = block_info.number;
            self->speed_data_.time = now;
          }

          self->last_appended_.emplace(std::move(block_info));
          callback(outcome::success());
        });
  }

}  // namespace kagome::consensus
