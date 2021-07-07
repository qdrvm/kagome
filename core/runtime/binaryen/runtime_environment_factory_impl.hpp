/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_RUNTIME_ENVIRONMENT_FACTORY_IMPL
#define KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_RUNTIME_ENVIRONMENT_FACTORY_IMPL

#include "runtime/binaryen/runtime_environment_factory.hpp"

#include "common/blob.hpp"
#include "crypto/hasher.hpp"
#include "host_api/host_api_factory.hpp"
#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "runtime/binaryen/module/wasm_module_factory.hpp"
#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/runtime_environment.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "runtime/runtime_code_provider.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "storage/trie/trie_batches.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::runtime::binaryen {

  class BinaryenWasmMemoryFactory;

  /**
   * @brief RuntimeEnvironmentFactory is a mechanism to prepare environment for
   * launching execute() function of runtime APIs. It supports in-memory cache
   * to reuse existing environments, avoid hi-load operations.
   */
  class RuntimeEnvironmentFactoryImpl final
      : public RuntimeEnvironmentFactory,
        public std::enable_shared_from_this<RuntimeEnvironmentFactoryImpl> {
   public:
    enum class Error { EMPTY_STATE_CODE = 1, NO_PERSISTENT_BATCH = 2 };

    RuntimeEnvironmentFactoryImpl(
        std::shared_ptr<CoreApiFactory> core_api_provider,
        std::shared_ptr<BinaryenWasmMemoryFactory> memory_factory,
        std::shared_ptr<host_api::HostApiFactory> host_api_factory,
        std::shared_ptr<WasmModuleFactory> module_factory,
        std::shared_ptr<RuntimeCodeProvider> wasm_provider,
        std::shared_ptr<TrieStorageProvider> storage_provider,
        std::shared_ptr<crypto::Hasher> hasher);

    outcome::result<RuntimeEnvironment> makeIsolated(
        const Config &config) override;

    outcome::result<RuntimeEnvironment> makePersistent() override;

    outcome::result<RuntimeEnvironment> makeEphemeral() override;

    outcome::result<RuntimeEnvironment> makeIsolatedAt(
        const storage::trie::RootHash &state_root,
        const Config &config) override;

    /**
     * @warning calling this with an \arg state_root older than the current root
     * will reset the storage to an older state once changes are committed
     */
    outcome::result<RuntimeEnvironment> makePersistentAt(
        const storage::trie::RootHash &state_root) override;

    outcome::result<RuntimeEnvironment> makeEphemeralAt(
        const storage::trie::RootHash &state_root) override;

   private:
    outcome::result<RuntimeEnvironment> createRuntimeEnvironment(
        const common::Buffer &state_code);

    outcome::result<RuntimeEnvironment> createIsolatedRuntimeEnvironment(
        const common::Buffer &state_code);

    log::Logger logger_ =
        log::createLogger("RuntimeEnvironmentFactory", "wasm");

    std::shared_ptr<CoreApiFactory> core_api_provider_;
    std::shared_ptr<TrieStorageProvider> storage_provider_;
    std::shared_ptr<RuntimeCodeProvider> wasm_provider_;
    std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
    std::shared_ptr<WasmModuleFactory> module_factory_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<BinaryenWasmMemoryFactory> memory_factory_;

    std::mutex modules_mutex_;
    std::map<common::Hash256, std::shared_ptr<WasmModule>> modules_;

    static thread_local std::shared_ptr<BinaryenMemoryProvider> memory_provider_;
    static thread_local std::shared_ptr<RuntimeExternalInterface>
        external_interface_;
  };

}  // namespace kagome::runtime::binaryen

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::binaryen,
                          RuntimeEnvironmentFactoryImpl::Error);

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_RUNTIME_ENVIRONMENT_FACTORY_IMPL
