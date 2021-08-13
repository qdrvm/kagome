/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_MODULE_REPOSITORY_IMPL_HPP
#define KAGOME_CORE_RUNTIME_MODULE_REPOSITORY_IMPL_HPP

#include "runtime/module_repository.hpp"

#include <unordered_map>

#include "log/logger.hpp"
#include "runtime/instance_environment.hpp"

namespace kagome::runtime {

  class RuntimeUpgradeTracker;
  class ModuleFactory;

  class ModuleRepositoryImpl final : public ModuleRepository {
   public:
    ModuleRepositoryImpl(
        std::shared_ptr<const RuntimeUpgradeTracker> runtime_upgrade_tracker,
        std::shared_ptr<const ModuleFactory> module_factory);

    outcome::result<
        std::pair<std::shared_ptr<ModuleInstance>, InstanceEnvironment>>
    getInstanceAt(std::shared_ptr<const RuntimeCodeProvider> code_provider,
                  const primitives::BlockInfo &block) override;

   private:
    std::unordered_map<storage::trie::RootHash, std::shared_ptr<Module>>
        modules_;
    std::mutex modules_mutex_;
    std::mutex instances_mutex_;
    std::shared_ptr<const RuntimeUpgradeTracker> runtime_upgrade_tracker_;
    std::shared_ptr<const ModuleFactory> module_factory_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_MODULE_REPOSITORY_IMPL_HPP
