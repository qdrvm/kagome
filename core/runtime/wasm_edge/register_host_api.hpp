/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <wasmedge/wasmedge.h>

#include <iostream>
#include <unordered_set>

#include "host_api/host_api.hpp"
#include "log/logger.hpp"
#include "runtime/common/register_host_api.hpp"

namespace kagome::runtime::wasm_edge {

  template <typename T>
  WasmEdge_ValType get_wasm_type() = delete;

  template <>
  WasmEdge_ValType get_wasm_type<int32_t>() {
    return WasmEdge_ValTypeGenI32();
  }

  template <>
  WasmEdge_ValType get_wasm_type<uint32_t>() {
    return WasmEdge_ValTypeGenI32();
  }

  template <>
  WasmEdge_ValType get_wasm_type<int64_t>() {
    return WasmEdge_ValTypeGenI64();
  }

  template <>
  WasmEdge_ValType get_wasm_type<uint64_t>() {
    return WasmEdge_ValTypeGenI64();
  }

  template <>
  WasmEdge_ValType get_wasm_type<float>() {
    return WasmEdge_ValTypeGenF32();
  }

  template <>
  WasmEdge_ValType get_wasm_type<double>() {
    return WasmEdge_ValTypeGenF64();
  }

  template <typename T>
  T get_wasm_value(WasmEdge_Value) = delete;

  template <>
  int64_t get_wasm_value<int64_t>(WasmEdge_Value v) {
    return WasmEdge_ValueGetI64(v);
  }

  template <>
  uint64_t get_wasm_value<uint64_t>(WasmEdge_Value v) {
    return WasmEdge_ValueGetI64(v);
  }

  template <>
  int32_t get_wasm_value<int32_t>(WasmEdge_Value v) {
    return WasmEdge_ValueGetI32(v);
  }

  template <>
  uint32_t get_wasm_value<uint32_t>(WasmEdge_Value v) {
    return WasmEdge_ValueGetI32(v);
  }

  template <typename Ret, typename... Args>
  using HostApiMethod = Ret (host_api::HostApi::*)(Args...);

  template <typename Ret, typename... Args>
  using ConstHostApiMethod = Ret (host_api::HostApi::*)(Args...) const;

  template <typename>
  struct HostApiMethodTraits;

  template <typename Ret_, typename... Args_>
  struct HostApiMethodTraits<HostApiMethod<Ret_, Args_...>> {
    using Ret = Ret_;
    using Args = std::tuple<Args_...>;
  };

  template <typename Ret_, typename... Args_>
  struct HostApiMethodTraits<ConstHostApiMethod<Ret_, Args_...>> {
    using Ret = Ret_;
    using Args = std::tuple<Args_...>;
  };

  template <typename F, typename... Args, size_t... Idxs>
  decltype(auto) call_with_array(
      F f,
      [[maybe_unused]] std::span<const WasmEdge_Value> array,
      std::index_sequence<Idxs...>) {
    return f(get_wasm_value<Args>(array[Idxs])...);
  }

  template <typename F, typename... Args>
  decltype(auto) call_with_array(F f,
                                 std::span<const WasmEdge_Value> array,
                                 std::tuple<Args...>) {
    return call_with_array<F, Args...>(
        f, array, std::make_index_sequence<sizeof...(Args)>());
  }

  template <auto Method>
  WasmEdge_Result host_method_wrapper(
      void *current_host_api,
      const WasmEdge_CallingFrameContext *call_frame_cxt,
      const WasmEdge_Value *params,
      WasmEdge_Value *returns) {
    using Ret = typename HostApiMethodTraits<decltype(Method)>::Ret;
    using Args = typename HostApiMethodTraits<decltype(Method)>::Args;
    BOOST_ASSERT(current_host_api);
    auto &host_api = *static_cast<host_api::HostApi *>(current_host_api);

    try {
      if constexpr (std::is_void_v<Ret>) {
        call_with_array(
            [&host_api](auto... params) mutable {
              std::invoke(Method, host_api, params...);
            },
            std::span{params, std::tuple_size_v<Args>},
            Args{});
      } else {
        Ret res = call_with_array(
            [&host_api](auto... params) mutable -> Ret {
              return std::invoke(Method, host_api, params...);
            },
            std::span{params, std::tuple_size_v<Args>},
            Args{});
        returns[0].Value = res;
        returns[0].Type = get_wasm_type<Ret>();
      }

    } catch (std::runtime_error &e) {
      auto logger = log::createLogger("HostApi", "runtime");
      SL_ERROR(logger, "Host API call failed with error: {}", e.what());
      return WasmEdge_Result_Fail;
    }
    return WasmEdge_Result_Success;
  }

  void register_method(WasmEdge_HostFunc_t cb,
                       WasmEdge_ModuleInstanceContext *module,
                       void *data,
                       std::string_view name,
                       std::span<WasmEdge_ValType> rets,
                       std::span<WasmEdge_ValType> args) {
    auto type = WasmEdge_FunctionTypeCreate(
        args.data(), args.size(), rets.data(), rets.size());
    auto instance = WasmEdge_FunctionInstanceCreate(type, cb, data, 0);
    WasmEdge_FunctionTypeDelete(type);

    auto name_str = WasmEdge_StringCreateByBuffer(name.data(), name.size());
    WasmEdge_ModuleInstanceAddFunction(module, name_str, instance);
    WasmEdge_StringDelete(name_str);
  }

  template <typename Ret, typename... Args>
  void register_method(WasmEdge_HostFunc_t cb,
                       WasmEdge_ModuleInstanceContext *module,
                       void *data,
                       std::string_view name) {
    std::array<WasmEdge_ValType, sizeof...(Args)> types{
        get_wasm_type<Args>()...};
    WasmEdge_ValType ret[1];
    std::span<WasmEdge_ValType> rets{};
    if constexpr (!std::is_void_v<Ret>) {
      ret[0] = get_wasm_type<Ret>();
      rets = std::span{ret, 1};
    }
    register_method(cb, module, data, name, rets, std::span(types));
  }

  template <auto Method, typename Ret, typename... Args>
  void register_host_method(WasmEdge_ModuleInstanceContext *module,
                            host_api::HostApi &host_api,
                            std::string_view name) {
    WasmEdge_HostFunc_t cb = &host_method_wrapper<Method>;
    register_method<Ret, Args...>(cb, module, &host_api, name);
  }

  WasmEdge_Result stub(void *data,
                       const WasmEdge_CallingFrameContext *,
                       const WasmEdge_Value *,
                       WasmEdge_Value *) {
    static log::Logger logger = log::createLogger("WasmEdge", "runtime");
    SL_ERROR(logger,
             "Attempt to call an unimplemented Host method '{}'",
             reinterpret_cast<const char *>(data));
    return WasmEdge_Result_Fail;
  }

  void stub_host_method(WasmEdge_ModuleInstanceContext *module,
                        std::string_view name,
                        std::span<WasmEdge_ValType> rets,
                        std::span<WasmEdge_ValType> args) {
    register_method(stub, module, (void *)name.data(), name, rets, args);
  }

#define REGISTER_HOST_METHOD(Ret, name, ...)            \
  register_host_method<&host_api::HostApi::name,        \
                       Ret __VA_OPT__(, ) __VA_ARGS__>( \
      instance, host_api, #name);                       \
  existing_imports.insert(#name);

  void register_host_api(host_api::HostApi &host_api,
                         WasmEdge_ASTModuleContext *module,
                         WasmEdge_ModuleInstanceContext *instance) {
    BOOST_ASSERT(instance);

    uint32_t imports_num = WasmEdge_ASTModuleListImportsLength(module);
    std::vector<WasmEdge_ImportTypeContext *> imports;
    imports.resize(imports_num);
    WasmEdge_ASTModuleListImports(
        module,
        const_cast<const WasmEdge_ImportTypeContext **>(imports.data()),
        imports_num);

    std::unordered_set<std::string_view> existing_imports;
    REGISTER_HOST_METHODS

    for (auto &import : imports) {
      auto name = WasmEdge_ImportTypeGetExternalName(import);
      auto type = WasmEdge_ImportTypeGetFunctionType(module, import);
      if (type) {
        std::string_view name_view{name.Buf, name.Length};
        if (!existing_imports.contains(name_view)) {
          std::vector<WasmEdge_ValType> args(
              WasmEdge_FunctionTypeGetParametersLength(type));
          std::vector<WasmEdge_ValType> rets(
              WasmEdge_FunctionTypeGetReturnsLength(type));
          WasmEdge_FunctionTypeGetParameters(type, args.data(), args.size());
          WasmEdge_FunctionTypeGetReturns(type, rets.data(), rets.size());

          stub_host_method(instance, name_view.data(), rets, args);
        }
      }
    }
  }

}  // namespace kagome::runtime::wasm_edge
