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

  thread_local ModuleRepositoryImpl::InstanceCache
      ModuleRepositoryImpl::instances_cache_;

  ModuleRepositoryImpl::ModuleRepositoryImpl(
      std::shared_ptr<const RuntimeUpgradeTracker> runtime_upgrade_tracker,
      std::shared_ptr<const ModuleFactory> module_factory)
      : runtime_upgrade_tracker_{std::move(runtime_upgrade_tracker)},
        module_factory_{std::move(module_factory)} {
    BOOST_ASSERT(runtime_upgrade_tracker_);
    BOOST_ASSERT(module_factory_);
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  ModuleRepositoryImpl::getInstanceAt(
      std::shared_ptr<const RuntimeCodeProvider> code_provider,
      const primitives::BlockInfo &block) {
    KAGOME_PROFILE_START(code_retrieval);
    OUTCOME_TRY(state, runtime_upgrade_tracker_->getLastCodeUpdateState(block));
    KAGOME_PROFILE_END(code_retrieval);

    KAGOME_PROFILE_START(module_retrieval);
    std::shared_ptr<Module> module;
    {
      std::lock_guard guard{modules_mutex_};
      if (auto it = modules_.find(state); it == modules_.end()) {
        OUTCOME_TRY(code, code_provider->getCodeAt(state));
        OUTCOME_TRY(new_module, module_factory_->make(code));
        module = std::move(new_module);
        modules_[state] = module;
      } else {
        module = it->second;
      }
    }
    KAGOME_PROFILE_END(module_retrieval);

    KAGOME_PROFILE_START(module_instantiation);
    {
      std::lock_guard guard{instances_mutex_};
      if (auto cached_instance = instances_cache_.get(state);
          not cached_instance.has_value()) {
        OUTCOME_TRY(instance, modules_[state]->instantiate());
        auto shared_instance =
            std::shared_ptr<ModuleInstance>(std::move(instance));
        BOOST_VERIFY(instances_cache_.put(state, shared_instance));
        KAGOME_PROFILE_END(module_instantiation);
        return shared_instance;
      } else {
        return cached_instance.value();
      }
    }
  }

}  // namespace kagome::runtime
