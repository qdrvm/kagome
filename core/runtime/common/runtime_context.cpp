/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_context.hpp"

#include "log/profiling_logger.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"

namespace kagome::runtime {
  using namespace kagome::common::literals;

  RuntimeContext::RuntimeContext(
      std::shared_ptr<ModuleInstance> module_instance)
      : module_instance{std::move(module_instance)} {
    BOOST_ASSERT(this->module_instance);
  }

  outcome::result<RuntimeContext> RuntimeContext::fromCode(
      const runtime::ModuleFactory &module_factory,
      common::BufferView code_zstd,
      ContextParams params) {
    common::Buffer code;
    OUTCOME_TRY(runtime::uncompressCodeIfNeeded(code_zstd, code));
    OUTCOME_TRY(runtime_module, module_factory.make(code));
    OUTCOME_TRY(instance, runtime_module->instantiate());
    runtime::RuntimeContext ctx{
        instance,
    };
    instance->getEnvironment()
        .storage_provider->setToEphemeralAt(storage::trie::kEmptyRootHash)
        .value();
    OUTCOME_TRY(instance->resetMemory(params.memory_limits));
    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContext::fromBatch(
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

  outcome::result<RuntimeContext> RuntimeContext::persistent(
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

  outcome::result<RuntimeContext> RuntimeContext::ephemeral(
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

}  // namespace kagome::runtime
