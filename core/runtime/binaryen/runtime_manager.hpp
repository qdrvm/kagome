/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_RUNTIME_MANAGER
#define KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_RUNTIME_MANAGER

#include <binaryen/wasm-interpreter.h>

#include "common/blob.hpp"
#include "common/logger.hpp"
#include "crypto/hasher.hpp"
#include "extensions/extension_factory.hpp"
#include "outcome/outcome.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "runtime/wasm_provider.hpp"
#include "runtime/trie_storage_provider.hpp"

namespace kagome::runtime::binaryen {

  /**
   * @brief RuntimeManager is a mechanism to prepare environment for launching
   * execute() function of runtime APIs. It supports in-memory cache to reuse
   * existing environments, avoid hi-load operations.
   */
  class RuntimeManager {
   public:
    enum class Error { EMPTY_STATE_CODE = 1, INVALID_STATE_CODE };

    RuntimeManager(
        std::shared_ptr<WasmProvider> wasm_provider,
        std::shared_ptr<extensions::ExtensionFactory> extension_factory,
        std::shared_ptr<TrieStorageProvider> storage_provider,
        std::shared_ptr<crypto::Hasher> hasher);

    using RuntimeEnvironment = std::tuple<std::shared_ptr<wasm::ModuleInstance>,
                                          std::shared_ptr<WasmMemory>>;

    outcome::result<RuntimeEnvironment> createPersistentRuntimeEnvironment();

    outcome::result<RuntimeEnvironment> createEphemeralRuntimeEnvironment();

    /**
     * @warning calling this with an \arg state_root older than the current root
     * will reset the storage to an older state once changes are committed
     */
    outcome::result<RuntimeEnvironment> createPersistentRuntimeEnvironmentAt(
        const common::Hash256 &state_root);

    outcome::result<RuntimeEnvironment> createEphemeralRuntimeEnvironmentAt(
        const common::Hash256 &state_root);

   private:
    outcome::result<RuntimeEnvironment>
    createRuntimeEnvironment();

    outcome::result<std::shared_ptr<wasm::Module>> prepareModule(
        const common::Buffer &state_code);

    common::Logger logger_ = common::createLogger("Runtime manager");

    std::shared_ptr<runtime::WasmProvider> wasm_provider_;
    std::shared_ptr<TrieStorageProvider> storage_provider_;
    std::shared_ptr<extensions::ExtensionFactory> extension_factory_;
    std::shared_ptr<crypto::Hasher> hasher_;

    // hash of WASM state code
    common::Hash256 state_code_hash_{};

    std::mutex modules_mutex_;
    std::map<common::Hash256, std::shared_ptr<wasm::Module>> modules_;

    std::mutex ei_mutex_;
    std::map<std::thread::id, std::shared_ptr<RuntimeExternalInterface>>
        external_interfaces_;
  };

}  // namespace kagome::runtime::binaryen

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::binaryen, RuntimeManager::Error);

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_RUNTIME_MANAGER
