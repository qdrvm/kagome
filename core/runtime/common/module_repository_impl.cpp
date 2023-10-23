/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/module_repository_impl.hpp"

#include "log/profiling_logger.hpp"
#include "runtime/common/runtime_instances_pool.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_code_provider.hpp"
#include "runtime/runtime_upgrade_tracker.hpp"

namespace kagome::runtime {
  using kagome::primitives::ThreadNumber;
  using soralog::util::getThreadNumber;

  ModuleRepositoryImpl::ModuleRepositoryImpl(
      std::shared_ptr<RuntimeInstancesPool> runtime_instances_pool,
      std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker,
      std::shared_ptr<const ModuleFactory> module_factory,
      std::shared_ptr<SingleModuleCache> last_compiled_module,
      std::shared_ptr<const RuntimeCodeProvider> code_provider)
      : runtime_instances_pool_{std::move(runtime_instances_pool)},
        runtime_upgrade_tracker_{std::move(runtime_upgrade_tracker)},
        module_factory_{std::move(module_factory)},
        last_compiled_module_{std::move(last_compiled_module)},
        code_provider_{code_provider},
        logger_{log::createLogger("Module Repository", "runtime")} {
    BOOST_ASSERT(runtime_instances_pool_);
    BOOST_ASSERT(runtime_upgrade_tracker_);
    BOOST_ASSERT(module_factory_);
    BOOST_ASSERT(last_compiled_module_);
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  ModuleRepositoryImpl::getInstanceAt(
      const primitives::BlockInfo &block,
      const storage::trie::RootHash &storage_state) {
    KAGOME_PROFILE_START(code_retrieval)
    OUTCOME_TRY(state, runtime_upgrade_tracker_->getLastCodeUpdateState(block));
    KAGOME_PROFILE_END(code_retrieval)

    KAGOME_PROFILE_START(module_retrieval) {
      // Add compiled module if any
      if (auto module = last_compiled_module_->try_extract();
          module.has_value()) {
        runtime_instances_pool_->putModule(state, module.value());
      }

      // Compile new module if required
      if (auto opt_module = runtime_instances_pool_->getModule(state);
          !opt_module.has_value()) {
        SL_DEBUG(logger_, "Runtime module cache miss for state {}", state);
        auto code = code_provider_->getCodeAt(state);
        if (not code.has_value()) {
          code = code_provider_->getCodeAt(storage_state);
        }
        if (not code.has_value()) {
          return code.as_failure();
        }
        OUTCOME_TRY(new_module,
                    module_factory_->make(code.value()));
        runtime_instances_pool_->putModule(state, std::move(new_module));
      }
    }

    // Try acquire instance (instantiate if needed)
    OUTCOME_TRY(runtime_instance, runtime_instances_pool_->tryAcquire(state));
    KAGOME_PROFILE_END(module_retrieval)

    return runtime_instance;
  }
}  // namespace kagome::runtime
