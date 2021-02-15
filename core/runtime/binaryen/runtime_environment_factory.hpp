/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_RUNTIME_ENVIRONMENT_FACTORY
#define KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_RUNTIME_ENVIRONMENT_FACTORY

#include "common/blob.hpp"
#include "common/logger.hpp"
#include "crypto/hasher.hpp"
#include "host_api/host_api_factory.hpp"
#include "outcome/outcome.hpp"
#include "runtime/binaryen/module/wasm_module_factory.hpp"
#include "runtime/binaryen/runtime_environment.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "runtime/wasm_provider.hpp"
#include "storage/trie/trie_batches.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::runtime::binaryen {

  /**
   * @brief RuntimeManager is a mechanism to prepare environment for launching
   * execute() function of runtime APIs. It supports in-memory cache to reuse
   * existing environments, avoid hi-load operations.
   */
  class RuntimeEnvironmentFactory {
   public:
    enum class Error { EMPTY_STATE_CODE = 1, NO_PERSISTENT_BATCH = 2 };

    RuntimeEnvironmentFactory(
        std::shared_ptr<host_api::HostApiFactory> host_api_factory,
        std::shared_ptr<WasmModuleFactory> module_factory,
        std::shared_ptr<WasmProvider> wasm_provider,
        std::shared_ptr<TrieStorageProvider> storage_provider,
        std::shared_ptr<crypto::Hasher> hasher);

    enum class CallPersistency {
      PERSISTENT,  // the changes made by this call will be applied to the state
                   // trie storage
      EPHEMERAL  // the changes made by this call will vanish once it's
                 // completed
    };

    outcome::result<RuntimeEnvironment> makeIsolated();

    outcome::result<RuntimeEnvironment> makePersistent();

    outcome::result<RuntimeEnvironment> makeEphemeral();

    /**
     * @warning calling this with an \arg state_root older than the current root
     * will reset the storage to an older state once changes are committed
     */
    outcome::result<RuntimeEnvironment> makePersistentAt(
        const common::Hash256 &state_root);

    outcome::result<RuntimeEnvironment> makeEphemeralAt(
        const common::Hash256 &state_root);

   private:
    outcome::result<RuntimeEnvironment> createRuntimeEnvironment(
        const common::Buffer &state_code);

    outcome::result<RuntimeEnvironment> createIsolatedRuntimeEnvironment(
        const common::Buffer &state_code);

    common::Logger logger_ =
        common::createLogger("Runtime environment factory");

    std::shared_ptr<TrieStorageProvider> storage_provider_;
    std::shared_ptr<WasmProvider> wasm_provider_;
    std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
    std::shared_ptr<WasmModuleFactory> module_factory_;
    std::shared_ptr<crypto::Hasher> hasher_;

    std::mutex modules_mutex_;
    std::map<common::Hash256, std::shared_ptr<WasmModule>> modules_;

    static thread_local std::shared_ptr<RuntimeExternalInterface>
        external_interface_;
  };

}  // namespace kagome::runtime::binaryen

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::binaryen,
                          RuntimeEnvironmentFactory::Error);

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_RUNTIME_ENVIRONMENT_FACTORY
