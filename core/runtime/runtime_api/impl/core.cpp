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
#include "runtime/module_repository.hpp"

namespace kagome::runtime {
  RestrictedCoreImpl::RestrictedCoreImpl(RuntimeContext ctx)
      : ctx_{std::move(ctx)} {}

  outcome::result<primitives::Version> RestrictedCoreImpl::version() {
    return ctx_.module_instance
        ->callAndDecodeExportFunction<primitives::Version>(ctx_,
                                                           "Core_version");
  }

  CoreImpl::CoreImpl(
      std::shared_ptr<Executor> executor,
      std::shared_ptr<ModuleRepository> module_repository,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker)
      : executor_{std::move(executor)},
        module_repository_{std::move(module_repository)},
        header_repo_{std::move(header_repo)},
        runtime_upgrade_tracker_{std::move(runtime_upgrade_tracker)} {
    BOOST_ASSERT(executor_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
    BOOST_ASSERT(runtime_upgrade_tracker_ != nullptr);
  }

  outcome::result<primitives::Version> CoreImpl::version(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(version, module_repository_->embeddedVersion(block));
    if (version) {
      return *version;
    }
    return version_.call(*header_repo_,
                         *runtime_upgrade_tracker_,
                         *executor_,
                         block,
                         "Core_version");
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
                executor_->ctx().persistentAt(block.header.parent_hash,
                                              std::move(changes_tracker)));
    return executor_->call<void>(ctx, "Core_execute_block", block);
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

  Core::InitializeBlockResult CoreImpl::initialize_block(
      const primitives::BlockHeader &header,
      TrieChangesTrackerOpt changes_tracker) {
    OUTCOME_TRY(version, this->version(header.parent_hash));
    auto core_version = primitives::detail::coreVersionFromApis(version.apis);
    OUTCOME_TRY(ctx,
                executor_->ctx().persistentAt(header.parent_hash,
                                              std::move(changes_tracker)));
    auto mode = ExtrinsicInclusionMode::AllExtrinsics;
    auto call = [&]<typename T>() {
      return executor_->call<T>(ctx, "Core_initialize_block", header);
    };
    if (core_version and core_version >= 5) {
      BOOST_OUTCOME_TRY(mode, call.operator()<ExtrinsicInclusionMode>());
    } else {
      OUTCOME_TRY(call.operator()<void>());
    }
    return std::make_pair(std::make_unique<RuntimeContext>(std::move(ctx)),
                          mode);
  }

}  // namespace kagome::runtime
