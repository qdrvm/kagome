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
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo)
      : executor_{std::move(executor)}, header_repo_{std::move(header_repo)} {
    BOOST_ASSERT(executor_ != nullptr);
  }

  outcome::result<primitives::Version> CoreImpl::version(
      RuntimeEnvironment &env) {
    return executor_->call<primitives::Version>(env, "Core_version");
  }

  outcome::result<primitives::Version> CoreImpl::version(
      const primitives::BlockHash &block) {
    return executor_->callAt<primitives::Version>(block, "Core_version");
  }

  outcome::result<primitives::Version> CoreImpl::version() {
    return executor_->callAtGenesis<primitives::Version>("Core_version");
  }

  outcome::result<void> CoreImpl::execute_block_ref(
      const primitives::BlockReflection &block,
      TrieChangesTrackerOpt changes_tracker) {
    BOOST_ASSERT([&] {
      auto parent_res = header_repo_->getBlockHeader(block.header.parent_hash);
      return parent_res.has_value()
         and parent_res.value().number == block.header.number - 1;
    }());
    OUTCOME_TRY(env,
                executor_->persistentAt(block.header.parent_hash,
                                        std::move(changes_tracker)));
    OUTCOME_TRY(executor_->call<void>(*env, "Core_execute_block", block));
    return outcome::success();
  }

  outcome::result<void> CoreImpl::execute_block(
      const primitives::Block &block, TrieChangesTrackerOpt changes_tracker) {
    return execute_block_ref(
        primitives::BlockReflection{
            .header =
                primitives::BlockHeaderReflection{
                    .parent_hash = block.header.parent_hash,
                    .number = block.header.number,
                    .state_root = block.header.state_root,
                    .extrinsics_root = block.header.extrinsics_root,
                    .digest = gsl::span<primitives::const DigestItem>(
                        block.header.digest.data(), block.header.digest.size()),
                },
            .body = block.body,
        },
        std::move(changes_tracker));
  }

  outcome::result<std::unique_ptr<RuntimeEnvironment>>
  CoreImpl::initialize_block(const primitives::BlockHeader &header,
                             TrieChangesTrackerOpt changes_tracker) {
    OUTCOME_TRY(env,
                executor_->persistentAt(header.parent_hash,
                                        std::move(changes_tracker)));
    OUTCOME_TRY(executor_->call<void>(*env, "Core_initialize_block", header));
    return std::move(env);
  }

}  // namespace kagome::runtime
