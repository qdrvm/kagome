/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_IMPL_CORE_HPP
#define KAGOME_RUNTIME_IMPL_CORE_HPP

#include "runtime/runtime_api/core.hpp"

#include "runtime/runtime_api/impl/lru.hpp"

namespace kagome::runtime {

  class Executor;
  class RuntimePropertiesCache;

  outcome::result<primitives::Version> callCoreVersion(
      const ModuleFactory &module_factory,
      common::BufferView code,
      const std::shared_ptr<RuntimePropertiesCache> &runtime_properties_cache);

  class CoreImpl final : public Core {
   public:
    CoreImpl(
        std::shared_ptr<Executor> executor,
        std::shared_ptr<RuntimeContextFactory> ctx_factory,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker);

    outcome::result<primitives::Version> version(
        const primitives::BlockHash &block) override;

    outcome::result<primitives::Version> version() override;

    outcome::result<void> execute_block(
        const primitives::Block &block,
        TrieChangesTrackerOpt changes_tracker) override;

    outcome::result<void> execute_block_ref(
        const primitives::BlockReflection &block,
        TrieChangesTrackerOpt changes_tracker) override;

    outcome::result<std::unique_ptr<RuntimeContext>> initialize_block(
        const primitives::BlockHeader &header,
        TrieChangesTrackerOpt changes_tracker) override;

   private:
    std::shared_ptr<Executor> executor_;
    std::shared_ptr<RuntimeContextFactory> ctx_factory_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker_;

    RuntimeApiLruCode<primitives::Version> version_{10};
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_IMPL_CORE_HPP
