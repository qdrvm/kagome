/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WASMEDGE_MODULE_INSTANCE_IMPL
#define KAGOME_CORE_RUNTIME_WASMEDGE_MODULE_INSTANCE_IMPL

#include "runtime/module_instance.hpp"

#include "log/logger.hpp"

struct WasmEdge_InterpreterContext;
struct WasmEdge_VMContext;

namespace kagome::runtime::wasmedge {

  class ModuleImpl;

  class ModuleInstanceImpl final : public ModuleInstance {
   public:
    ModuleInstanceImpl(InstanceEnvironment &&env,
                       std::shared_ptr<const ModuleImpl> parent,
                       WasmEdge_VMContext *rei);
    outcome::result<PtrSize> callExportFunction(std::string_view name,
                                                PtrSize args) const override;

    outcome::result<boost::optional<WasmValue>> getGlobal(
        std::string_view name) const override;

    InstanceEnvironment const &getEnvironment() const override;

    outcome::result<void> resetEnvironment() override;

   private:
    InstanceEnvironment env_;
    WasmEdge_InterpreterContext *interpreter_;
    WasmEdge_VMContext* rei_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wasmedge

#endif  // KAGOME_CORE_RUNTIME_WASMEDGE_MODULE_INSTANCE_IMPL
