/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
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
  class Memory;

  class RuntimeContext {
   public:
    // should be created from runtime contex factory
    RuntimeContext() = delete;
    RuntimeContext(const RuntimeContext &) = delete;
    RuntimeContext &operator=(const RuntimeContext &) = delete;

    RuntimeContext(RuntimeContext &&) noexcept = default;
    RuntimeContext &operator=(RuntimeContext &&) noexcept = delete;

    ~RuntimeContext();

    // https://github.com/paritytech/polkadot-sdk/blob/e16ef0861f576dd260487d78b57949b18795ed77/polkadot/primitives/src/v6/executor_params.rs#L32
    static constexpr size_t DEFAULT_STACK_MAX = 65536;
    static constexpr bool DEFAULT_WASM_EXT_BULK_MEMORY = false;

    struct ContextParams {
      MemoryLimits memory_limits;
      bool wasm_ext_bulk_memory = DEFAULT_WASM_EXT_BULK_MEMORY;
      bool operator==(const ContextParams &other) const = default;
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

    static outcome::result<RuntimeContext> stateless(
        std::shared_ptr<ModuleInstance> instance);

    virtual outcome::result<RuntimeContext> fromBatch(
        std::shared_ptr<ModuleInstance> module_instance,
        std::shared_ptr<storage::trie::TrieBatch> batch) const = 0;

    virtual outcome::result<RuntimeContext> persistent(
        std::shared_ptr<ModuleInstance> module_instance,
        const storage::trie::RootHash &state,
        std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
            changes_tracker_opt) const = 0;

    virtual outcome::result<RuntimeContext> persistentAt(
        const primitives::BlockHash &block_hash,
        std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
            changes_tracker_opt) const = 0;

    virtual outcome::result<RuntimeContext> ephemeral(
        std::shared_ptr<ModuleInstance> module_instance,
        const storage::trie::RootHash &state) const = 0;

    virtual outcome::result<RuntimeContext> ephemeralAt(
        const primitives::BlockHash &block_hash) const = 0;

    virtual outcome::result<RuntimeContext> ephemeralAt(
        const primitives::BlockHash &block_hash,
        const storage::trie::RootHash &state) const = 0;

    virtual outcome::result<RuntimeContext> ephemeralAtGenesis() const = 0;
  };

  class RuntimeContextFactoryImpl : public RuntimeContextFactory {
   public:
    using ContextParams = RuntimeContext::ContextParams;

    explicit RuntimeContextFactoryImpl(
        std::shared_ptr<ModuleRepository> module_repo,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo);

    outcome::result<RuntimeContext> fromBatch(
        std::shared_ptr<ModuleInstance> module_instance,
        std::shared_ptr<storage::trie::TrieBatch> batch) const override;

    outcome::result<RuntimeContext> persistent(
        std::shared_ptr<ModuleInstance> module_instance,
        const storage::trie::RootHash &state,
        std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
            changes_tracker_opt) const override;

    outcome::result<RuntimeContext> persistentAt(
        const primitives::BlockHash &block_hash,
        std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
            changes_tracker_opt = {}) const override;

    outcome::result<RuntimeContext> ephemeral(
        std::shared_ptr<ModuleInstance> module_instance,
        const storage::trie::RootHash &state) const override;

    outcome::result<RuntimeContext> ephemeralAt(
        const primitives::BlockHash &block_hash) const override;

    outcome::result<RuntimeContext> ephemeralAt(
        const primitives::BlockHash &block_hash,
        const storage::trie::RootHash &state) const override;

    outcome::result<RuntimeContext> ephemeralAtGenesis() const override;

   private:
    std::shared_ptr<class ModuleRepository> module_repo_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
  };

}  // namespace kagome::runtime

SCALE_TIE_HASH_STD(kagome::runtime::RuntimeContext::ContextParams);
