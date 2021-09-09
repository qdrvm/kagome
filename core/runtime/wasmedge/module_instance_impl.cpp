/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasmedge/module_instance_impl.hpp"

#include "host_api/host_api.hpp"

#include <wasmedge.h>

namespace kagome::runtime::wasmedge {

  ModuleInstanceImpl::ModuleInstanceImpl(InstanceEnvironment &&env,
                                         WasmEdge_ASTModuleContext *parent,
                                         WasmEdge_ImportObjectContext *rei)

      : env_{std::move(env)},
        logger_{log::createLogger("ModuleInstance", "wasmedge")} {
    interpreter_ = WasmEdge_InterpreterCreate(nullptr, nullptr);
    store_ = WasmEdge_StoreCreate();
    auto ModName = WasmEdge_StringCreateByCString("ext");
    auto Res = WasmEdge_InterpreterRegisterModule(
        interpreter_, store_, parent, ModName);
    Res = WasmEdge_InterpreterRegisterImport(interpreter_, store_, rei);
    WasmEdge_StringDelete(ModName);
  }

  outcome::result<PtrSize> ModuleInstanceImpl::callExportFunction(
      std::string_view name, PtrSize args) const {
    auto FuncName = WasmEdge_StringCreateByCString(name.data());
    WasmEdge_Value Params[2] = {WasmEdge_ValueGenI32(args.ptr),
                                WasmEdge_ValueGenI32(args.size)};
    WasmEdge_Value Returns[1];
    auto Res = WasmEdge_InterpreterInvoke(
        interpreter_, store_, FuncName, Params, 2, Returns, 1);
    WasmEdge_StringDelete(FuncName);
    return PtrSize{WasmEdge_ValueGetI64(Returns[0])};
  }

  outcome::result<boost::optional<WasmValue>> ModuleInstanceImpl::getGlobal(
      std::string_view name) const {
    auto GlobalName = WasmEdge_StringCreateByCString(name.data());
    auto res = WasmEdge_StoreFindGlobal(store_, GlobalName);
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
        return boost::none;
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
