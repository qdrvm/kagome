/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <utility>

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
#include "runtime/wabt/instrument.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"

namespace kagome::runtime {
  RuntimeContext::RuntimeContext(
      std::shared_ptr<ModuleInstance> module_instance)
      : module_instance{std::move(module_instance)} {
    BOOST_ASSERT(this->module_instance);
  }

  RuntimeContext::~RuntimeContext() = default;

  RuntimeContextFactoryImpl::RuntimeContextFactoryImpl(
      std::shared_ptr<ModuleRepository> module_repo,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo)
      : module_repo_{std::move(module_repo)},
        header_repo_{std::move(header_repo)} {}

  outcome::result<RuntimeContext> RuntimeContextFactory::stateless(
      std::shared_ptr<ModuleInstance> instance) {
    OUTCOME_TRY(instance->stateless());
    return RuntimeContext{instance};
  }

  outcome::result<RuntimeContext> RuntimeContextFactoryImpl::fromBatch(
      std::shared_ptr<ModuleInstance> instance,
      std::shared_ptr<storage::trie::TrieBatch> batch) const {
    runtime::RuntimeContext ctx{
        instance,
    };
    instance->getEnvironment().storage_provider->setTo(batch);
    OUTCOME_TRY(instance->resetMemory());
    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContextFactoryImpl::persistent(
      std::shared_ptr<ModuleInstance> instance,
      const storage::trie::RootHash &state,
      std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
          changes_tracker_opt) const {
    runtime::RuntimeContext ctx{
        instance,
    };
    OUTCOME_TRY(instance->getEnvironment().storage_provider->setToPersistentAt(
        state, changes_tracker_opt));
    OUTCOME_TRY(instance->resetMemory());
    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContextFactoryImpl::ephemeral(
      std::shared_ptr<ModuleInstance> instance,
      const storage::trie::RootHash &state) const {
    runtime::RuntimeContext ctx{
        instance,
    };
    OUTCOME_TRY(
        instance->getEnvironment().storage_provider->setToEphemeralAt(state));
    OUTCOME_TRY(instance->resetMemory());
    return ctx;
  }

  outcome::result<RuntimeContext>
  RuntimeContextFactoryImpl::ephemeralAtGenesis() const {
    OUTCOME_TRY(genesis_hash, header_repo_->getHashByNumber(0));
    OUTCOME_TRY(genesis_header, header_repo_->getBlockHeader(genesis_hash));
    OUTCOME_TRY(instance,
                module_repo_->getInstanceAt({genesis_hash, 0},
                                            genesis_header.state_root));

    OUTCOME_TRY(ctx, ephemeral(instance, genesis_header.state_root));

    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContextFactoryImpl::persistentAt(
      const primitives::BlockHash &block_hash,
      TrieChangesTrackerOpt changes_tracker) const {
    OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
    OUTCOME_TRY(instance,
                module_repo_->getInstanceAt({block_hash, header.number},
                                            header.state_root));

    OUTCOME_TRY(ctx, persistent(instance, header.state_root, changes_tracker));

    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContextFactoryImpl::ephemeralAt(
      const primitives::BlockHash &block_hash) const {
    OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
    OUTCOME_TRY(instance,
                module_repo_->getInstanceAt({block_hash, header.number},
                                            header.state_root));

    OUTCOME_TRY(ctx, ephemeral(instance, header.state_root));

    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContextFactoryImpl::ephemeralAt(
      const primitives::BlockHash &block_hash,
      const storage::trie::RootHash &state_hash) const {
    OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
    OUTCOME_TRY(instance,
                module_repo_->getInstanceAt({block_hash, header.number},
                                            header.state_root));

    OUTCOME_TRY(ctx, ephemeral(instance, state_hash));

    return ctx;
  }

}  // namespace kagome::runtime
