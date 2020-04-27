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
        std::shared_ptr<runtime::WasmProvider> wasm_provider,
        std::shared_ptr<extensions::ExtensionFactory> extension_factory,
        std::shared_ptr<crypto::Hasher> hasher);

    outcome::result<std::tuple<std::shared_ptr<wasm::ModuleInstance>,
        std::shared_ptr<WasmMemory>>>
    getPersistentRuntimeEnvironment();

    outcome::result<std::tuple<std::shared_ptr<wasm::ModuleInstance>,
        std::shared_ptr<WasmMemory>>>
    getEphemeralRuntimeEnvironment();

   private:
    outcome::result<std::tuple<std::shared_ptr<wasm::ModuleInstance>,
        std::shared_ptr<WasmMemory>>>
    getRuntimeEnvironment();

    outcome::result<std::shared_ptr<wasm::Module>> prepareModule(
        const common::Buffer &state_code);

    common::Logger logger_ = common::createLogger("Runtime manager");
    std::shared_ptr<runtime::WasmProvider> wasm_provider_;
    std::shared_ptr<extensions::ExtensionFactory> extension_factory_;

    std::shared_ptr<crypto::Hasher> hasher_;
    common::Hash256 state_code_hash_{};

    // TODO (xDimon): Separate it by state_code_hash for multiruntime case
    std::shared_ptr<wasm::Module> module_{};

    // TODO (xDimon): Separate it per thread in multithread environment
    std::shared_ptr<RuntimeExternalInterface> external_interface_{};
  };

}  // namespace kagome::runtime::binaryen

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::binaryen, RuntimeManager::Error);

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_RUNTIME_API_RUNTIME_MANAGER
