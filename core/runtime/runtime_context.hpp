/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"
#include "runtime/types.hpp"
#include "storage/trie/types.hpp"

namespace kagome::storage::trie {
  class TrieBatch;
}

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::storage::changes_trie {
  class ChangesTracker;
}

namespace kagome::runtime {
  class ModuleFactory;
  class ModuleInstance;
  class ModuleRepository;

  class RuntimeContext {
   public:
    // should be created from runtime contex factory
    RuntimeContext() = delete;
    RuntimeContext(const RuntimeContext &) = delete;
    RuntimeContext &operator=(const RuntimeContext &) = delete;

    RuntimeContext(RuntimeContext &&) = default;

    // constructor for tests
    static RuntimeContext create_TEST(
        std::shared_ptr<ModuleInstance> module_instance) {
      return RuntimeContext{module_instance};
    }

    struct ContextParams {
      MemoryLimits memory_limits;
    };

    const std::shared_ptr<ModuleInstance> module_instance;

   private:
    friend class RuntimeContextFactoryImpl;
    friend class RuntimeContextFactory;
    RuntimeContext(std::shared_ptr<ModuleInstance> module_instance);
  };

  class RuntimeContextFactory {
   public:
    using ContextParams = RuntimeContext::ContextParams;

    virtual ~RuntimeContextFactory() = default;

    static outcome::result<RuntimeContext> fromCode(
        const runtime::ModuleFactory &module_factory,
        common::BufferView code_zstd,
        ContextParams params = {});

    virtual outcome::result<RuntimeContext> fromBatch(
        std::shared_ptr<ModuleInstance> module_instance,
        std::shared_ptr<storage::trie::TrieBatch> batch,
        ContextParams params = {}) = 0;

    virtual outcome::result<RuntimeContext> persistent(
        std::shared_ptr<ModuleInstance> module_instance,
        const storage::trie::RootHash &state,
        std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
            changes_tracker_opt,
        ContextParams params = {}) = 0;

    virtual outcome::result<RuntimeContext> persistentAt(
        const primitives::BlockHash &block_hash,
        std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
            changes_tracker_opt,
        ContextParams params = {}) = 0;

    virtual outcome::result<RuntimeContext> ephemeral(
        std::shared_ptr<ModuleInstance> module_instance,
        const storage::trie::RootHash &state,
        ContextParams params = {}) = 0;

    virtual outcome::result<RuntimeContext> ephemeralAt(
        const primitives::BlockHash &block_hash, ContextParams params = {}) = 0;

    virtual outcome::result<RuntimeContext> ephemeralAt(
        const primitives::BlockHash &block_hash,
        const storage::trie::RootHash &state,
        ContextParams params = {}) = 0;

    virtual outcome::result<RuntimeContext> ephemeralAtGenesis(
        ContextParams params = {}) = 0;
  };

  class RuntimeContextFactoryImpl : public RuntimeContextFactory {
   public:
    using ContextParams = RuntimeContext::ContextParams;

    explicit RuntimeContextFactoryImpl(
        std::shared_ptr<ModuleRepository> module_repo,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo);

    virtual outcome::result<RuntimeContext> fromBatch(
        std::shared_ptr<ModuleInstance> module_instance,
        std::shared_ptr<storage::trie::TrieBatch> batch,
        ContextParams params = {}) override;

    virtual outcome::result<RuntimeContext> persistent(
        std::shared_ptr<ModuleInstance> module_instance,
        const storage::trie::RootHash &state,
        std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
            changes_tracker_opt,
        ContextParams params = {}) override;

    virtual outcome::result<RuntimeContext> persistentAt(
        const primitives::BlockHash &block_hash,
        std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
            changes_tracker_opt,
        ContextParams params = {}) override;

    virtual outcome::result<RuntimeContext> ephemeral(
        std::shared_ptr<ModuleInstance> module_instance,
        const storage::trie::RootHash &state,
        ContextParams params = {}) override;

    virtual outcome::result<RuntimeContext> ephemeralAt(
        const primitives::BlockHash &block_hash,
        ContextParams params = {}) override;

    virtual outcome::result<RuntimeContext> ephemeralAt(
        const primitives::BlockHash &block_hash,
        const storage::trie::RootHash &state,
        ContextParams params = {}) override;

    virtual outcome::result<RuntimeContext> ephemeralAtGenesis(
        ContextParams params = {}) override;

   private:
    std::shared_ptr<class ModuleRepository> module_repo_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
  };

}  // namespace kagome::runtime
