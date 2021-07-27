/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "module_repository_impl.hpp"

#include "runtime/module.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/runtime_code_provider.hpp"
#include "runtime/runtime_upgrade_tracker.hpp"

namespace kagome::runtime {

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
    OUTCOME_TRY(state, runtime_upgrade_tracker_->getLastCodeUpdateState(block));

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

    {
      std::lock_guard guard{instances_mutex_};
      if (auto it = instances_.find(state); it == instances_.end()) {
        OUTCOME_TRY(instance, modules_[state]->instantiate());
        std::shared_ptr<ModuleInstance> shared_instance = std::move(instance);
        instances_[state] = shared_instance;
        return shared_instance;
      } else {
        return it->second;
      }
    }
  }

}  // namespace kagome::runtime
