/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_HPP
#define KAGOME_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_HPP

#include "blockchain/block_header_repository.hpp"
#include "common/buffer.hpp"
#include "host_api/host_api.hpp"
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

  class RuntimeEnvironment {
   public:
    RuntimeEnvironment(
        std::shared_ptr<const ModuleInstance> module_instance,
        Memory &memory,
        boost::optional<std::shared_ptr<storage::trie::PersistentTrieBatch>>
            batch,
        std::function<void(RuntimeEnvironment &)> on_destruction);

    ~RuntimeEnvironment();

    std::shared_ptr<const ModuleInstance> module_instance;
    Memory &memory;
    boost::optional<std::shared_ptr<storage::trie::PersistentTrieBatch>> batch;

   private:
    std::function<void(RuntimeEnvironment &)> on_destruction_;
  };

  class RuntimeEnvironmentFactory
      : public std::enable_shared_from_this<RuntimeEnvironmentFactory> {
   public:
    enum class Error {
      PARENT_FACTORY_EXPIRED = 1,
      ABSENT_BLOCK,
      ABSENT_HEAP_BASE,
      FAILED_TO_SET_STORAGE_STATE
    };
    struct RuntimeEnvironmentTemplate;

    RuntimeEnvironmentFactory(
        std::shared_ptr<TrieStorageProvider> storage_provider,
        std::shared_ptr<host_api::HostApi> host_api,
        std::shared_ptr<MemoryProvider> memory_provider,
        std::shared_ptr<const runtime::RuntimeCodeProvider> code_provider,
        std::shared_ptr<ModuleRepository> module_repo,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo);

    virtual ~RuntimeEnvironmentFactory() = default;

    void setEnvCleanupCallback(
        std::function<void(RuntimeEnvironment &)> &&callback);

    [[nodiscard]] virtual std::unique_ptr<RuntimeEnvironmentTemplate> start();

   private:
    primitives::BlockInfo latest_state_;
    std::shared_ptr<TrieStorageProvider> storage_provider_;
    std::shared_ptr<host_api::HostApi> host_api_;
    std::shared_ptr<MemoryProvider> memory_provider_;
    std::shared_ptr<const runtime::RuntimeCodeProvider> code_provider_;
    std::shared_ptr<ModuleRepository> module_repo_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
    log::Logger logger_;
    std::function<void(RuntimeEnvironment &)> env_cleanup_callback_;
  };

  struct RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate {
   public:
    RuntimeEnvironmentTemplate(
        std::weak_ptr<RuntimeEnvironmentFactory> parent_factory_);

    virtual ~RuntimeEnvironmentTemplate() = default;

    [[nodiscard]] virtual RuntimeEnvironmentTemplate &setState(
        const primitives::BlockInfo &state);

    [[nodiscard]] virtual RuntimeEnvironmentTemplate &persistent();

    [[nodiscard]] virtual outcome::result<std::unique_ptr<RuntimeEnvironment>>
    make();

   private:
    primitives::BlockInfo state_;
    std::weak_ptr<RuntimeEnvironmentFactory> parent_factory_;
    bool persistent_{false};
  };

}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, RuntimeEnvironmentFactory::Error);

#endif  // KAGOME_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_HPP
