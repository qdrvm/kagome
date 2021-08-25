/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_MODULE_REPOSITORY_IMPL_HPP
#define KAGOME_CORE_RUNTIME_MODULE_REPOSITORY_IMPL_HPP

#include "runtime/module_repository.hpp"

#include <queue>

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

    outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt(
        std::shared_ptr<const RuntimeCodeProvider> code_provider,
        const primitives::BlockInfo &block) override;

   private:
    std::unordered_map<storage::trie::RootHash, std::shared_ptr<Module>>
        modules_;
    std::mutex modules_mutex_;
    // ring-buffer cache for runtime module instances
    static constexpr size_t INSTANCES_CACHE_SIZE = 8;

    struct InstanceCache {
     public:
      struct InstanceCacheEntry {
        std::shared_ptr<ModuleInstance> instance;
        mutable uint8_t priority;
        storage::trie::RootHash state_hash;

        bool operator<(const InstanceCacheEntry &rhs) const {
          return priority < rhs.priority;
        }
      };

      boost::optional<std::shared_ptr<ModuleInstance>> get(
          const storage::trie::RootHash &state_hash) const {
        for (auto &entry : cache_) {
          if (entry.state_hash == state_hash) {
            if (entry.priority < UINT8_MAX) {
              entry.priority++;
            }
            return entry.instance;
          }
        }
        return boost::none;
      }

      [[nodiscard]] bool put(const storage::trie::RootHash &state_hash,
                             std::shared_ptr<ModuleInstance> instance) {
        cache_.push_back(
            InstanceCacheEntry{std::move(instance), 0, std::move(state_hash)});
        std::push_heap(cache_.begin(), cache_.end());
        if (cache_.size() > INSTANCES_CACHE_SIZE) {
          std::pop_heap(cache_.begin(), cache_.end());
          cache_.pop_back();
        }
        return true;
      }

     private:
      std::vector<InstanceCacheEntry> cache_;
    };

    static thread_local InstanceCache instances_cache_;
    std::mutex instances_mutex_;
    std::shared_ptr<const RuntimeUpgradeTracker> runtime_upgrade_tracker_;
    std::shared_ptr<const ModuleFactory> module_factory_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_MODULE_REPOSITORY_IMPL_HPP
