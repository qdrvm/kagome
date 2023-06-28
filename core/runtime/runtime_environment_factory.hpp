/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_HPP
#define KAGOME_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_HPP

#include "blockchain/block_header_repository.hpp"
#include "common/buffer.hpp"
#include "host_api/host_api.hpp"
#include "host_api/host_api_factory.hpp"
#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "primitives/version.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/module_repository.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "scale/scale.hpp"
#include "storage/trie/trie_batches.hpp"

namespace kagome::runtime {
  class ModuleFactory;

  class RuntimeEnvironment {
   public:
    RuntimeEnvironment(std::shared_ptr<ModuleInstance> module_instance,
                       std::shared_ptr<const MemoryProvider> memory_provider,
                       std::shared_ptr<TrieStorageProvider> storage_provider,
                       primitives::BlockInfo blockchain_state);

    static outcome::result<RuntimeEnvironment> fromCode(
        const runtime::ModuleFactory &module_factory,
        common::BufferView code_zstd);

    const std::shared_ptr<ModuleInstance> module_instance;
    const std::shared_ptr<const MemoryProvider> memory_provider;
    const std::shared_ptr<TrieStorageProvider> storage_provider;
  };

  outcome::result<void> resetMemory(const ModuleInstance &module);

  class RuntimeEnvironmentFactory
      : public std::enable_shared_from_this<RuntimeEnvironmentFactory> {
   public:
    enum class Error {
      PARENT_FACTORY_EXPIRED = 1,
      ABSENT_BLOCK,
      ABSENT_HEAP_BASE,
      HEAP_BASE_TOO_LOW,
      FAILED_TO_SET_STORAGE_STATE
    };

    struct RuntimeEnvironmentTemplate;

    RuntimeEnvironmentFactory(
        std::shared_ptr<const runtime::RuntimeCodeProvider> code_provider,
        std::shared_ptr<ModuleRepository> module_repo,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo);

    virtual ~RuntimeEnvironmentFactory() = default;

    /**
     * @param blockchain_state - the block to take the runtime code from
     * @param storage_state
     *        need to store separately from
     *        blockchain state because, for example, when we're in
     *        process of producing a block, there is no particular
     *        storage state associated with the block
     * @return a RuntimeEnvironmentTemplate which can be used to configure and
     * produce a RuntimeEnvironment
     */
    [[nodiscard]] virtual std::unique_ptr<RuntimeEnvironmentTemplate> start(
        const primitives::BlockInfo &blockchain_state,
        const storage::trie::RootHash &storage_state) const;

    /**
     * @brief returns a handle to make a RuntimeEnvironment at the state of the
     * provided block
     * @param blockchain_state - the block to take the runtime code from
     * @return a RuntimeEnvironmentTemplate which can be used to configure and
     * produce a RuntimeEnvironment
     */
    [[nodiscard]] virtual outcome::result<
        std::unique_ptr<RuntimeEnvironmentTemplate>>
    start(const primitives::BlockHash &blockchain_state) const;

    /**
     * @brief returns a handle to make a RuntimeEnvironment at genesis block
     * state
     * @return a RuntimeEnvironmentTemplate which can be used to configure and
     * produce a RuntimeEnvironment
     */
    [[nodiscard]] virtual outcome::result<
        std::unique_ptr<RuntimeEnvironmentTemplate>>
    start() const;

   private:
    std::shared_ptr<const runtime::RuntimeCodeProvider> code_provider_;
    std::shared_ptr<ModuleRepository> module_repo_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
    log::Logger logger_;
  };

  struct RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate {
   public:
    RuntimeEnvironmentTemplate(
        std::weak_ptr<const RuntimeEnvironmentFactory> parent_factory_,
        const primitives::BlockInfo &blockchain_state,
        const storage::trie::RootHash &storage_state);

    virtual ~RuntimeEnvironmentTemplate() = default;

    [[nodiscard]] virtual RuntimeEnvironmentTemplate &persistent();
    [[nodiscard]] virtual RuntimeEnvironmentTemplate &withStorageBatch(
        std::shared_ptr<storage::trie::TrieBatch> batch);
    [[nodiscard]] virtual RuntimeEnvironmentTemplate &withChangesTracker(
        TrieChangesTrackerOpt changes_tracker);

    [[nodiscard]] virtual outcome::result<std::unique_ptr<RuntimeEnvironment>>
    make();

   private:
    primitives::BlockInfo blockchain_state_;

    // need to store separately from
    // blockchain state because, for example, when we're in
    // process of producing a block, there is no particular
    // storage state associated with the block
    storage::trie::RootHash storage_state_;

    std::weak_ptr<const RuntimeEnvironmentFactory> parent_factory_;
    bool persistent_{false};
    std::shared_ptr<storage::trie::TrieBatch> batch_;
    TrieChangesTrackerOpt changes_tracker_;
  };

}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, RuntimeEnvironmentFactory::Error);

#endif  // KAGOME_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_HPP
