/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/core.hpp"

#include "blockchain/block_header_repository.hpp"
#include "log/logger.hpp"
#include "runtime/common/executor.hpp"

namespace kagome::runtime {

  CoreImpl::CoreImpl(
      std::shared_ptr<Executor> executor,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo)
      : executor_{std::move(executor)},
        changes_tracker_{std::move(changes_tracker)},
        header_repo_{std::move(header_repo)} {
    BOOST_ASSERT(executor_ != nullptr);
    BOOST_ASSERT(changes_tracker_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
  }

  outcome::result<primitives::Version> CoreImpl::version(
      const primitives::BlockHash &block) {
    return executor_->callAt<primitives::Version>(block, "Core_version");
  }

  outcome::result<primitives::Version> CoreImpl::version() {
    return executor_->callAtGenesis<primitives::Version>("Core_version");
  }

  outcome::result<void> CoreImpl::execute_block(
      const primitives::Block &block) {
    BOOST_ASSERT([&] {
      auto parent_res = header_repo_->getBlockHeader(block.header.parent_hash);
      return parent_res.has_value()
             and parent_res.value().number == block.header.number - 1;
    }());
    changes_tracker_->onBlockExecutionStart(block.header.parent_hash);
    OUTCOME_TRY(env, executor_->persistentAt(block.header.parent_hash));
    OUTCOME_TRY(executor_->call<void>(*env, "Core_execute_block", block));
    return outcome::success();
  }

  outcome::result<std::unique_ptr<RuntimeEnvironment>>
  CoreImpl::initialize_block(const primitives::BlockHeader &header) {
    changes_tracker_->onBlockExecutionStart(header.parent_hash);
    OUTCOME_TRY(env, executor_->persistentAt(header.parent_hash));
    OUTCOME_TRY(executor_->call<void>(*env, "Core_initialize_block", header));
    return std::move(env);
  }

}  // namespace kagome::runtime
