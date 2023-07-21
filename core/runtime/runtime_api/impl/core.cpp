/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/core.hpp"

#include "blockchain/block_header_repository.hpp"
#include "log/logger.hpp"
#include "runtime/executor.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_properties_cache.hpp"

namespace kagome::runtime {

  CoreImpl::CoreImpl(
      std::shared_ptr<Executor> executor,
      std::shared_ptr<RuntimeContextFactory> ctx_factory,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo)
      : executor_{std::move(executor)},
        ctx_factory_{std::move(ctx_factory)},
        header_repo_{std::move(header_repo)} {
    BOOST_ASSERT(executor_ != nullptr);
    BOOST_ASSERT(ctx_factory_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
  }

  outcome::result<primitives::Version> CoreImpl::version(
      std::shared_ptr<ModuleInstance> instance) {
    OUTCOME_TRY(genesis_hash, header_repo_->getHashByNumber(0));
    OUTCOME_TRY(genesis_header, header_repo_->getBlockHeader(genesis_hash));
    OUTCOME_TRY(ctx,
                ctx_factory_->ephemeral(instance, genesis_header.state_root));
    return executor_->decodedCallWithCtx<primitives::Version>(ctx,
                                                              "Core_version");
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
    OUTCOME_TRY(ctx,
                ctx_factory_->persistentAt(block.header.parent_hash,
                                           std::move(changes_tracker)));
    OUTCOME_TRY(
        executor_->decodedCallWithCtx<void>(ctx, "Core_execute_block", block));
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
                    .digest = gsl::span<const primitives::DigestItem>(
                        block.header.digest.data(), block.header.digest.size()),
                },
            .body = block.body,
        },
        std::move(changes_tracker));
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
