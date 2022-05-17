/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_INSTANCE_IMPL
#define KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_INSTANCE_IMPL

#include "runtime/module_instance.hpp"

#include "log/logger.hpp"

namespace wasm {
  using namespace ::wasm;  // NOLINT(google-build-using-namespace)
  class Module;
  class ModuleInstance;
}  // namespace wasm

namespace kagome::runtime::binaryen {

  class RuntimeExternalInterface;

  class ModuleInstanceImpl final : public ModuleInstance {
   public:
    enum class Error {
      UNEXPECTED_EXIT = 1,
      EXECUTION_ERROR,
      CAN_NOT_OBTAIN_GLOBAL
    };

    ModuleInstanceImpl(InstanceEnvironment &&env,
                       std::shared_ptr<wasm::Module> parent,
                       std::shared_ptr<RuntimeExternalInterface> rei);

    outcome::result<PtrSize> callExportFunction(std::string_view name,
                                                PtrSize args) const override;

    outcome::result<std::optional<WasmValue>> getGlobal(
        std::string_view name) const override;

    InstanceEnvironment const &getEnvironment() const override;

    outcome::result<void> resetEnvironment() override;
    outcome::result<void> borrow(std::function<void()>) override;

   private:
    InstanceEnvironment env_;
    std::shared_ptr<RuntimeExternalInterface> rei_;
    std::shared_ptr<wasm::Module>
        parent_;  // must be kept alive because binaryen's module instance keeps
                  // a reference to it
    std::unique_ptr<wasm::ModuleInstance> module_instance_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::binaryen

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::binaryen, ModuleInstanceImpl::Error);

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_INSTANCE_IMPL
