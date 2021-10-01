/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/module_repository_impl.hpp"

#include "log/profiling_logger.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_code_provider.hpp"
#include "runtime/runtime_upgrade_tracker.hpp"

namespace kagome::runtime {

  thread_local SmallLruCache<storage::trie::RootHash,
                             std::shared_ptr<ModuleInstance>>
      ModuleRepositoryImpl::instances_cache_{
          ModuleRepositoryImpl::INSTANCES_CACHE_SIZE};

  ModuleRepositoryImpl::ModuleRepositoryImpl(
      std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker,
      std::shared_ptr<const ModuleFactory> module_factory)
      : modules_{MODULES_CACHE_SIZE},
        runtime_upgrade_tracker_{std::move(runtime_upgrade_tracker)},
        module_factory_{std::move(module_factory)},
        logger_{log::createLogger("Module Repository", "runtime")} {
    BOOST_ASSERT(runtime_upgrade_tracker_);
    BOOST_ASSERT(module_factory_);
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  ModuleRepositoryImpl::getInstanceAt(
      std::shared_ptr<const RuntimeCodeProvider> code_provider,
      const primitives::BlockInfo &block) {
    KAGOME_PROFILE_START(code_retrieval)
    OUTCOME_TRY(state, runtime_upgrade_tracker_->getLastCodeUpdateState(block));
    KAGOME_PROFILE_END(code_retrieval)

    auto cached_instance = instances_cache_.get(state);
    if (cached_instance.has_value()) {
      return cached_instance.value();
    }

    KAGOME_PROFILE_START(module_retrieval)
    std::shared_ptr<Module> module;
    {
      std::lock_guard guard{modules_mutex_};
      if (auto opt_module = modules_.get(state); !opt_module.has_value()) {
        OUTCOME_TRY(code, code_provider->getCodeAt(state));
        OUTCOME_TRY(new_module, module_factory_->make(code));
        module = std::move(new_module);
        BOOST_VERIFY(modules_.put(state, module));
      } else {
        module = opt_module.value();
      }
    }
    KAGOME_PROFILE_END(module_retrieval)

    KAGOME_PROFILE_START(module_instantiation)
    std::shared_ptr<ModuleInstance> shared_instance;
    {
      std::lock_guard guard{instances_mutex_};
      OUTCOME_TRY(instance, module->instantiate());
      shared_instance = std::move(instance);
    }
    BOOST_VERIFY(instances_cache_.put(state, shared_instance));

    KAGOME_PROFILE_END(module_instantiation)
    return shared_instance;
  }

}  // namespace kagome::runtime
