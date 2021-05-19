/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE_IMPL
#define KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE_IMPL

#include "common/buffer.hpp"
#include "runtime/binaryen/module/wasm_module.hpp"

namespace wasm {
  using namespace ::wasm;  // NOLINT(google-build-using-namespace)
  class Module;
}  // namespace wasm

namespace kagome::runtime::binaryen {

  /**
   * Stores a wasm::Module and a wasm::Module instance which contains the module
   * and the provided runtime external interface
   */
  class WasmModuleImpl final : public WasmModule, public std::enable_shared_from_this<WasmModuleImpl> {
   public:
    static constexpr auto kDefaultHeappages = 1024;
    enum class Error { EMPTY_STATE_CODE = 1, INVALID_STATE_CODE };

    WasmModuleImpl(WasmModuleImpl &&) = default;
    WasmModuleImpl &operator=(WasmModuleImpl &&) = default;

    WasmModuleImpl(const WasmModuleImpl &) = delete;
    WasmModuleImpl &operator=(const WasmModuleImpl &) = delete;

    ~WasmModuleImpl() override = default;

    static outcome::result<std::unique_ptr<WasmModuleImpl>> createFromCode(
        const common::Buffer &code,
        const std::shared_ptr<RuntimeExternalInterface> &rei,
        const std::shared_ptr<TrieStorageProvider> &storage_provider);

    std::unique_ptr<WasmModuleInstance> instantiate(
        const std::shared_ptr<RuntimeExternalInterface> &externalInterface)
        const override;

   private:
    explicit WasmModuleImpl(std::unique_ptr<wasm::Module> &&module);

    std::shared_ptr<wasm::Module> module_; // shared to module instances
  };

}  // namespace kagome::runtime::binaryen

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::binaryen, WasmModuleImpl::Error);

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE_IMPL
