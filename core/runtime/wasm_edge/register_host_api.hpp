/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <wasmedge/wasmedge.h>

#include <iostream>

#include "host_api/host_api.hpp"
#include "runtime/common/register_host_api.hpp"

namespace kagome::runtime::wasm_edge {

  template <typename T>
  WasmEdge_ValType get_wasm_type() = delete;

  template <>
  WasmEdge_ValType get_wasm_type<int32_t>() {
    return WasmEdge_ValType_I32;
  }

  template <>
  WasmEdge_ValType get_wasm_type<int64_t>() {
    return WasmEdge_ValType_I64;
  }

  template <>
  WasmEdge_ValType get_wasm_type<float>() {
    return WasmEdge_ValType_F32;
  }

  template <>
  WasmEdge_ValType get_wasm_type<double>() {
    return WasmEdge_ValType_F64;
  }

  template <typename T>
  T get_wasm_value(WasmEdge_Value) = delete;

  template <>
  int64_t get_wasm_value<int64_t>(WasmEdge_Value v) {
    return WasmEdge_ValueGetI64(v);
  }

  template <>
  int32_t get_wasm_value<int32_t>(WasmEdge_Value v) {
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
      auto log = log::createLogger("HostApi", "runtime");
      SL_ERROR(log, "Host API call failed with error: {}", e.what());
      return WasmEdge_Result_Terminate;
    }
    return WasmEdge_Result_Success;
  }

  template <typename Ret, typename... Args>
  void register_method(WasmEdge_HostFunc_t cb,
                       WasmEdge_ModuleInstanceContext *module,
                       void *data,
                       std::string_view name) {
    WasmEdge_ValType types[]{get_wasm_type<Args>()...};
    WasmEdge_ValType ret[1];
    WasmEdge_ValType *ret_ptr;
    if constexpr (std::is_void_v<Ret>) {
      ret_ptr = nullptr;
    } else {
      ret[0] = get_wasm_type<Ret>();
      ret_ptr = ret;
    }
    auto type = WasmEdge_FunctionTypeCreate(
        types, sizeof...(Args), ret_ptr, ret_ptr == nullptr ? 0 : 1);
    auto instance = WasmEdge_FunctionInstanceCreate(type, cb, data, 0);
    WasmEdge_FunctionTypeDelete(type);

    WasmEdge_ModuleInstanceAddFunction(
        module,
        WasmEdge_StringCreateByBuffer(name.data(), name.size()),
        instance);
  }

  template <auto Method, typename Ret, typename... Args>
  void register_host_method(WasmEdge_ModuleInstanceContext *module,
                            host_api::HostApi &host_api,
                            std::string_view name) {
    WasmEdge_HostFunc_t cb = &host_method_wrapper<Method>;
    register_method<Ret, Args...>(cb, module, &host_api, name);
  }

#define REGISTER_HOST_METHOD(Ret, name, ...)            \
  register_host_method<&host_api::HostApi::name,        \
                       Ret __VA_OPT__(, ) __VA_ARGS__>( \
      instance, host_api, #name);

  void register_host_api(host_api::HostApi &host_api,
                         WasmEdge_ModuleInstanceContext *instance) {
    BOOST_ASSERT(instance);

    REGISTER_HOST_METHODS
  }

}  // namespace kagome::runtime::wasm_edge
