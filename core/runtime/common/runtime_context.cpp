/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_context.hpp"

#include "blockchain/block_header_repository.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/module_repository.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"

namespace kagome::runtime {
  using namespace kagome::common::literals;

  RuntimeContext::RuntimeContext(
      std::shared_ptr<ModuleInstance> module_instance)
      : module_instance{std::move(module_instance)} {
    BOOST_ASSERT(this->module_instance);
  }

  RuntimeContextFactoryImpl::RuntimeContextFactoryImpl(
      std::shared_ptr<ModuleRepository> module_repo,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo)
      : module_repo_{module_repo}, header_repo_{header_repo} {}

  outcome::result<RuntimeContext> RuntimeContextFactory::fromCode(
      const runtime::ModuleFactory &module_factory,
      common::BufferView code_zstd,
      ContextParams params) {
    common::Buffer code;
    OUTCOME_TRY(runtime::uncompressCodeIfNeeded(code_zstd, code));
    auto runtime_module_res = module_factory.make(code);
    if (!runtime_module_res) {
      return Error::COMPILATION_FAILED;
    }
    auto instance = runtime_module_res.value()->instantiate();
    runtime::RuntimeContext ctx{
        instance,
    };
    instance->getEnvironment()
        .storage_provider->setToEphemeralAt(storage::trie::kEmptyRootHash)
        .value();
    OUTCOME_TRY(instance->resetMemory(params.memory_limits));
    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContextFactoryImpl::fromBatch(
      std::shared_ptr<ModuleInstance> instance,
      std::shared_ptr<storage::trie::TrieBatch> batch,
      ContextParams params) {
    runtime::RuntimeContext ctx{
        instance,
    };
    instance->getEnvironment().storage_provider->setTo(batch);
    OUTCOME_TRY(instance->resetMemory(params.memory_limits));
    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContextFactoryImpl::persistent(
      std::shared_ptr<ModuleInstance> instance,
      const storage::trie::RootHash &state,
      std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
          changes_tracker_opt,
      ContextParams params) {
    runtime::RuntimeContext ctx{
        instance,
    };
    OUTCOME_TRY(instance->getEnvironment().storage_provider->setToPersistentAt(
        state, changes_tracker_opt));
    OUTCOME_TRY(instance->resetMemory(params.memory_limits));
    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContextFactoryImpl::ephemeral(
      std::shared_ptr<ModuleInstance> instance,
      const storage::trie::RootHash &state,
      ContextParams params) {
    runtime::RuntimeContext ctx{
        instance,
    };
    OUTCOME_TRY(
        instance->getEnvironment().storage_provider->setToEphemeralAt(state));
    OUTCOME_TRY(instance->resetMemory(params.memory_limits));
    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContextFactoryImpl::ephemeralAtGenesis(
      ContextParams params) {
    OUTCOME_TRY(genesis_hash, header_repo_->getHashByNumber(0));
    OUTCOME_TRY(genesis_header, header_repo_->getBlockHeader(genesis_hash));
    OUTCOME_TRY(instance,
                module_repo_->getInstanceAt({genesis_hash, 0},
                                            genesis_header.state_root));

    OUTCOME_TRY(ctx, ephemeral(instance, genesis_header.state_root, params));

    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContextFactoryImpl::persistentAt(
      const primitives::BlockHash &block_hash,
      TrieChangesTrackerOpt changes_tracker,
      ContextParams params) {
    OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
    OUTCOME_TRY(instance,
                module_repo_->getInstanceAt({block_hash, header.number},
                                            header.state_root));

    OUTCOME_TRY(
        ctx, persistent(instance, header.state_root, changes_tracker, params));

    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContextFactoryImpl::ephemeralAt(
      const primitives::BlockHash &block_hash, ContextParams params) {
    OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
    OUTCOME_TRY(instance,
                module_repo_->getInstanceAt({block_hash, header.number},
                                            header.state_root));

    OUTCOME_TRY(ctx, ephemeral(instance, header.state_root, params));

    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContextFactoryImpl::ephemeralAt(
      const primitives::BlockHash &block_hash,
      const storage::trie::RootHash &state_hash,
      ContextParams params) {
    OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
    OUTCOME_TRY(instance,
                module_repo_->getInstanceAt({block_hash, header.number},
                                            header.state_root));

    OUTCOME_TRY(ctx, ephemeral(instance, state_hash, params));

    return ctx;
  }

}  // namespace kagome::runtime
