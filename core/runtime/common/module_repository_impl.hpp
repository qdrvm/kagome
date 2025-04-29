/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/module_repository.hpp"

#include <thread>
#include <unordered_map>

#include "log/logger.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/runtime_context.hpp"
#include "utils/lru.hpp"
#include "utils/safe_object.hpp"

namespace kagome::blockchain {
  class BlockHeaderRepository;
}  // namespace kagome::blockchain

namespace kagome::crypto {
  class Hasher;
}  // namespace kagome::crypto

namespace kagome::storage::trie {
  class TrieStorage;
}  // namespace kagome::storage::trie

namespace kagome::runtime {
  class RuntimeUpgradeTracker;
  class ModuleFactory;
  class RuntimeInstancesPool;

  class ModuleRepositoryImpl final : public ModuleRepository {
   public:
    ModuleRepositoryImpl(
        std::shared_ptr<RuntimeInstancesPool> runtime_instances_pool,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<blockchain::BlockHeaderRepository>
            block_header_repository,
        std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker,
        std::shared_ptr<storage::trie::TrieStorage> trie_storage,
        std::shared_ptr<const ModuleFactory> module_factory,
        std::shared_ptr<const RuntimeCodeProvider> code_provider);

    outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt(
        const primitives::BlockInfo &block,
        const storage::trie::RootHash &state) override;

    outcome::result<std::optional<primitives::Version>> embeddedVersion(
        const primitives::BlockHash &block_hash) override;

   private:
    struct Item {
      common::Hash256 hash;
      std::shared_ptr<const common::Buffer> code;
      std::optional<primitives::Version> version;
      RuntimeContext::ContextParams ctx_params;
    };

    outcome::result<Item> codeAt(const primitives::BlockInfo &block,
                                 const storage::trie::RootHash &storage_state);

    std::shared_ptr<RuntimeInstancesPool> runtime_instances_pool_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repository_;
    std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker_;
    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;
    std::shared_ptr<const ModuleFactory> module_factory_;
    std::shared_ptr<const RuntimeCodeProvider> code_provider_;
    SafeObject<Lru<common::Hash256, Item>> cache_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime
