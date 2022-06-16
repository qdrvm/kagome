/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_MODULE_REPOSITORY_IMPL_HPP
#define KAGOME_CORE_RUNTIME_MODULE_REPOSITORY_IMPL_HPP

#include "runtime/module_repository.hpp"

#include <thread>
#include <unordered_map>

#include "log/logger.hpp"
#include "runtime/instance_environment.hpp"

namespace kagome::runtime {
  class RuntimeUpgradeTracker;
  class ModuleFactory;
  class SingleModuleCache;
  class RuntimeInstancesPool;

  class ModuleRepositoryImpl final : public ModuleRepository {
   public:
    ModuleRepositoryImpl(
        std::shared_ptr<RuntimeInstancesPool> runtime_instances_pool,
        std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker,
        std::shared_ptr<const ModuleFactory> module_factory,
        std::shared_ptr<SingleModuleCache> last_compiled_module);

    outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt(
        std::shared_ptr<const RuntimeCodeProvider> code_provider,
        const primitives::BlockInfo &block,
        const primitives::BlockHeader &header) override;

   private:
    std::shared_ptr<RuntimeInstancesPool> runtime_instances_pool_;
    std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker_;
    std::shared_ptr<const ModuleFactory> module_factory_;
    std::shared_ptr<SingleModuleCache> last_compiled_module_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_MODULE_REPOSITORY_IMPL_HPP
