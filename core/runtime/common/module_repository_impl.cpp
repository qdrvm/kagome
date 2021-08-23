/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/module_repository_impl.hpp"

#include "log/logger.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_code_provider.hpp"
#include "runtime/runtime_upgrade_tracker.hpp"

namespace kagome::runtime {

  thread_local std::unordered_map<storage::trie::RootHash,
                            std::shared_ptr<ModuleInstance>>
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
    SL_PROFILE_START(state_retrieval);
    OUTCOME_TRY(state, runtime_upgrade_tracker_->getLastCodeUpdateState(block));
    SL_PROFILE_END(state_retrieval);

    SL_PROFILE_START(module_retrieval);
    std::shared_ptr<Module> module;
    {
      std::lock_guard guard{modules_mutex_};
      if (auto it = modules_.find(state); it == modules_.end()) {
        OUTCOME_TRY(code, code_provider->getCodeAt(state));
        OUTCOME_TRY(new_module, module_factory_->make(state, code));
        module = std::move(new_module);
        modules_[state] = module;
      } else {
        module = it->second;
      }
    }
    SL_PROFILE_END(module_retrieval);

    SL_PROFILE_START(module_instantiation);
    {
      std::lock_guard guard{instances_mutex_};
      if (auto it = instances_cache_.find(state);
          it == instances_cache_.end()) {
        OUTCOME_TRY(instance, modules_[state]->instantiate());
        auto shared_instance =
            std::shared_ptr<ModuleInstance>(std::move(instance));
        SL_PROFILE_END(module_instantiation);
        auto emplaced_it = instances_cache_.emplace(
            std::make_pair(state, std::move(shared_instance)));
        BOOST_ASSERT(emplaced_it.second);
        return emplaced_it.first->second;
      } else {
        return it->second;
      }
    }
  }

}  // namespace kagome::runtime
