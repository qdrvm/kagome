/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/runtime_api/core.hpp"

#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  WavmCore::WavmCore(
      std::shared_ptr<Executor> executor,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo)
      : executor_{std::move(executor)},
        changes_tracker_{std::move(changes_tracker)},
        header_repo_{std::move(header_repo)} {
    BOOST_ASSERT(executor_ != nullptr);
    BOOST_ASSERT(changes_tracker_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
  }

  outcome::result<primitives::Version> WavmCore::versionAt(
      primitives::BlockHash const &block) {
    return executor_->callAt<primitives::Version>(block, "Core_version");
  }

  outcome::result<primitives::Version> WavmCore::version() {
    return executor_->nestedCall<primitives::Version>("Core_version");
  }

  outcome::result<void> WavmCore::execute_block(
      const primitives::Block &block) {
    OUTCOME_TRY(changes_tracker_->onBlockChange(
        block.header.parent_hash,
        block.header.number - 1));  // parent's number
    OUTCOME_TRY(executor_->startNewEnvironment(
        {block.header.number - 1, block.header.parent_hash}));
    return executor_->persistentCall<void>("Core_execute_block", block);
  }

  outcome::result<void> WavmCore::initialise_block(
      const primitives::BlockHeader &header) {
    OUTCOME_TRY(
        changes_tracker_->onBlockChange(header.parent_hash,
                                        header.number - 1));  // parent's number
    OUTCOME_TRY(executor_->startNewEnvironment(
        {header.number - 1, header.parent_hash}));
    return executor_->persistentCall<void>("Core_initialise_block", header);
  }

  outcome::result<std::vector<primitives::AuthorityId>> WavmCore::authorities(
      const primitives::BlockId &block_id) {
    return executor_->callAtLatest<std::vector<primitives::AuthorityId>>(
        "Core_authorities", block_id);
  }

}  // namespace kagome::runtime::wavm
