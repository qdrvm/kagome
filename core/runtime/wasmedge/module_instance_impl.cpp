/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasmedge/module_instance_impl.hpp"

#include "host_api/impl/host_api_impl.hpp"
#include "runtime/wasmedge/memory_provider.hpp"
#include "runtime/wasmedge/module_impl.hpp"
#include "runtime/wasmedge/register_host_api.hpp"

#include <wasmedge.h>

namespace kagome::runtime::wasmedge {

  ModuleInstanceImpl::ModuleInstanceImpl(InstanceEnvironment &&env,
                                         const ModuleImpl *parent)
      : env_{std::move(env)},
        parent_{parent},
        logger_{log::createLogger("ModuleInstance", "wasmedge")} {
    rei_ = WasmEdge_VMCreate(NULL, NULL);
    WasmEdge_String ModuleName = WasmEdge_StringCreateByCString("env");
    //    auto Res = WasmEdge_VMRegisterModuleFromASTModule(
    //        rei_, ModuleName, parent_->ast());
    WasmEdge_ImportObjectContext *ImpObj =
        WasmEdge_ImportObjectCreate(ModuleName, env_.host_api.get());
    //    WasmEdge_ImportObjectContext *ImpObj =
    //        WasmEdge_VMGetImportModuleContext(rei_,
    //        WasmEdge_HostRegistration_WasmEdge_Process);
    WasmEdge_StringDelete(ModuleName);
    register_host_api(ImpObj);
    dynamic_cast<WasmedgeMemoryProvider *>(env_.memory_provider.get())
        ->setExternalInterface(ImpObj);
    auto Res = WasmEdge_VMRegisterModuleFromImport(rei_, ImpObj);
    // interpreter_ = WasmEdge_InterpreterCreate(nullptr, nullptr);
    // auto store = WasmEdge_VMGetStoreContext(rei_);
    // WasmEdge_InterpreterInstantiate(interpreter_, store, parent->ast());
  }

  outcome::result<PtrSize> ModuleInstanceImpl::callExportFunction(
      std::string_view name, PtrSize args) const {
    auto FuncName = WasmEdge_StringCreateByCString(name.data());
    WasmEdge_Value Params[2] = {WasmEdge_ValueGenI32(args.ptr),
                                WasmEdge_ValueGenI32(args.size)};
    WasmEdge_Value Returns[1];
    // auto store = WasmEdge_VMGetStoreContext(rei_);
    auto Res = WasmEdge_VMRunWasmFromASTModule(
        rei_, parent_->ast(), FuncName, Params, 2, Returns, 1);
    // auto Res = WasmEdge_InterpreterInvoke(
    //     interpreter_, store, FuncName, Params, 2, Returns, 1);
    WasmEdge_StringDelete(FuncName);
    auto i = WasmEdge_ValueGetI64(Returns[0]);
    return PtrSize{i};
  }

  outcome::result<std::optional<WasmValue>> ModuleInstanceImpl::getGlobal(
      std::string_view name) const {
    auto GlobalName = WasmEdge_StringCreateByCString(name.data());
    auto store = WasmEdge_VMGetStoreContext(rei_);
    auto res = WasmEdge_StoreFindGlobal(store, GlobalName);
    auto type = WasmEdge_GlobalInstanceGetValType(res);
    auto val = WasmEdge_GlobalInstanceGetValue(res);
    WasmEdge_StringDelete(GlobalName);
    switch (type) {
      case WasmEdge_ValType_I32:
        return WasmValue{WasmEdge_ValueGetI32(val)};
      case WasmEdge_ValType_I64:
        return WasmValue{WasmEdge_ValueGetI64(val)};
      case WasmEdge_ValType_F32:
        return WasmValue{WasmEdge_ValueGetF32(val)};
      case WasmEdge_ValType_F64:
        return WasmValue{WasmEdge_ValueGetF64(val)};
      default:
        return std::nullopt;
    }
  }

  InstanceEnvironment const &ModuleInstanceImpl::getEnvironment() const {
    return env_;
  }

  outcome::result<void> ModuleInstanceImpl::resetEnvironment() {
    env_.host_api->reset();
    return outcome::success();
  }

}  // namespace kagome::runtime::wasmedge
