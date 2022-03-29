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

  ModuleRepositoryImpl::ModuleRepositoryImpl(
      std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker,
      std::shared_ptr<const ModuleFactory> module_factory,
      std::shared_ptr<SingleModuleCache> single_module_cache)
      : modules_{MODULES_CACHE_SIZE},
        runtime_upgrade_tracker_{std::move(runtime_upgrade_tracker)},
        module_factory_{std::move(module_factory)},
        single_module_cache_{std::move(single_module_cache)},
        logger_{log::createLogger("Module Repository", "runtime")} {
    BOOST_ASSERT(runtime_upgrade_tracker_);
    BOOST_ASSERT(module_factory_);
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  ModuleRepositoryImpl::getInstanceAt(
      std::shared_ptr<const RuntimeCodeProvider> code_provider,
      const primitives::BlockInfo &block,
      const primitives::BlockHeader &header) {
    KAGOME_PROFILE_START(code_retrieval)
    OUTCOME_TRY(state, runtime_upgrade_tracker_->getLastCodeUpdateState(block));
    KAGOME_PROFILE_END(code_retrieval)

    auto tid = std::this_thread::get_id();
    auto thread_local_cache = instances_cache_.end();
    {
      std::unique_lock lock{instances_mutex_};
      auto thread_cache = instances_cache_.find(tid);
      if (thread_cache == instances_cache_.end()) {
        // no cache was created for this thread
        auto insert_res =
            instances_cache_.emplace(tid, InstanceCache{INSTANCES_CACHE_SIZE});
        BOOST_ASSERT(insert_res.second);
        SL_DEBUG(logger_,
                 "Initialize new runtime instance cache for thread {}",
                 state.toHex(),
                 tid);
        thread_local_cache = insert_res.first;
      } else if (auto opt_instance = thread_cache->second.get(state);
                 opt_instance.has_value()) {
        SL_DEBUG(logger_,
                 "Found cached runtime instance for state {}, thread {}",
                 state.toHex(),
                 tid);
        return opt_instance.value();
      } else {
        SL_DEBUG(logger_,
                 "Runtime instance cache miss for state {}, thread {}",
                 state.toHex(),
                 tid);
        // there is a cache for this thread, but it doesn't have the required
        // instance
        thread_local_cache = thread_cache;
      }
    }
    // either found existed or emplaced a new one
    BOOST_ASSERT(thread_local_cache != instances_cache_.end());

    if (single_module_cache_ && single_module_cache_->module.has_value()) {
      BOOST_VERIFY(modules_.put(state, single_module_cache_->module.value()));
      single_module_cache_->module.reset();
    }

    KAGOME_PROFILE_START(module_retrieval)
    std::shared_ptr<Module> module;
    {
      std::lock_guard guard{modules_mutex_};
      if (auto opt_module = modules_.get(state); !opt_module.has_value()) {
        SL_DEBUG(
            logger_, "Runtime module cache miss for state {}", state.toHex());
        auto code = code_provider->getCodeAt(state);
        if (not code.has_value()) {
          code = code_provider->getCodeAt(header.state_root);
        }
        if (not code.has_value()) {
          return code.as_failure();
        }
        OUTCOME_TRY(new_module, module_factory_->make(code.value()));
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
    SL_DEBUG(logger_,
             "Instantiated a new runtime instance for state {}, thread",
             state.toHex(),
             tid);
    // thread_local_cache is an iterator into instances_cache_
    BOOST_VERIFY(thread_local_cache->second.put(state, shared_instance));

    KAGOME_PROFILE_END(module_instantiation)
    return shared_instance;
  }

}  // namespace kagome::runtime
