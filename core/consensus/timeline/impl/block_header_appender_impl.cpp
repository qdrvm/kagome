/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/block_header_appender_impl.hpp"

#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/timeline/impl/block_addition_error.hpp"
#include "consensus/timeline/impl/block_appender_base.hpp"
#include "consensus/timeline/timeline.hpp"

namespace kagome::consensus {

  BlockHeaderAppenderImpl::BlockHeaderAppenderImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<crypto::Hasher> hasher,
      std::unique_ptr<BlockAppenderBase> appender,
      LazySPtr<Timeline> timeline)
      : block_tree_{std::move(block_tree)},
        hasher_{std::move(hasher)},
        appender_{std::move(appender)},
        timeline_{timeline},
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
    auto block_info = block_header.blockInfo();

    if (block_tree_->has(block_info.hash)) {
      callback(outcome::success());
      return;
    }

    if (not block_tree_->has(block_header.parent_hash)) {
      SL_WARN(logger_, "Skipping a block {} with unknown parent", block_info);
      callback(BlockAdditionError::PARENT_NOT_FOUND);
      return;
    }

    // get current time to measure performance if block execution
    const auto t_start = std::chrono::high_resolution_clock::now();

    primitives::Block block{.header = std::move(block_header)};

    if (auto res = appender_->validateHeader(block); res.has_error()) {
      callback(res.as_failure());
      return;
    }

    if (auto res = block_tree_->addBlockHeader(block.header); res.has_error()) {
      callback(res.as_failure());
      return;
    }

    timeline_.get()->checkAndReportEquivocation(block.header);

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
            const auto time_delta_sec =
                std::chrono::duration_cast<std::chrono::seconds>(time_delta)
                    .count();
            SL_LOG(
                self->logger_,
                self->speed_data_.block_number ? log::Level::INFO
                                               : static_cast<log::Level>(-1),
                "Imported {} more headers of blocks {}-{}. "
                "Average speed is {} bps",
                block_delta,
                self->speed_data_.block_number,
                block_info.number,
                time_delta_sec != 0ull ? block_delta / time_delta_sec : 0ull);
            self->speed_data_.block_number = block_info.number;
            self->speed_data_.time = now;
          }

          callback(outcome::success());
        });
  }

}  // namespace kagome::consensus
