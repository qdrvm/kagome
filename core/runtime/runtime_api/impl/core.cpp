/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/core.hpp"

#include "blockchain/block_header_repository.hpp"
#include "log/logger.hpp"
#include "runtime/executor.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_properties_cache.hpp"

namespace kagome::runtime {
  outcome::result<primitives::Version> callCoreVersion(
      const ModuleFactory &module_factory,
      common::BufferView code,
      const std::shared_ptr<RuntimePropertiesCache> &runtime_properties_cache) {
    OUTCOME_TRY(ctx,
                runtime::RuntimeContextFactory::fromCode(module_factory, code));
    return Executor{nullptr, runtime_properties_cache}
        .decodedCallWithCtx<primitives::Version>(ctx, "Core_version");
  }

  CoreImpl::CoreImpl(
      std::shared_ptr<Executor> executor,
      std::shared_ptr<RuntimeContextFactory> ctx_factory,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker)
      : executor_{std::move(executor)},
        ctx_factory_{std::move(ctx_factory)},
        header_repo_{std::move(header_repo)},
        runtime_upgrade_tracker_{std::move(runtime_upgrade_tracker)} {
    BOOST_ASSERT(executor_ != nullptr);
    BOOST_ASSERT(ctx_factory_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
  }

  outcome::result<primitives::Version> CoreImpl::version(
      const primitives::BlockHash &block) {
    return version_.call(*header_repo_,
                         *runtime_upgrade_tracker_,
                         *executor_,
                         block,
                         "Core_version");
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
    OUTCOME_TRY(ctx,
                ctx_factory_->persistentAt(block.header.parent_hash,
                                           std::move(changes_tracker)));
    OUTCOME_TRY(
        executor_->decodedCallWithCtx<void>(ctx, "Core_execute_block", block));
    return outcome::success();
  }

  outcome::result<void> CoreImpl::execute_block(
      const primitives::Block &block, TrieChangesTrackerOpt changes_tracker) {
    primitives::BlockHeaderReflection header_ref(block.header);
    primitives::BlockReflection block_ref{
        .header = header_ref,
        .body = block.body,
    };
    return execute_block_ref(block_ref, std::move(changes_tracker));
  }

  outcome::result<std::unique_ptr<RuntimeContext>> CoreImpl::initialize_block(
      const primitives::BlockHeader &header,
      TrieChangesTrackerOpt changes_tracker) {
    OUTCOME_TRY(ctx,
                ctx_factory_->persistentAt(header.parent_hash,
                                           std::move(changes_tracker)));
    OUTCOME_TRY(executor_->decodedCallWithCtx<void>(
        ctx, "Core_initialize_block", header));
    return std::make_unique<RuntimeContext>(std::move(ctx));
  }

}  // namespace kagome::runtime
