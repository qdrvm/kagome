/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <wasmedge/wasmedge.h>

#include <iostream>

#include "host_api/host_api.hpp"

namespace kagome::runtime::wasm_edge {

  template <typename T>
  WasmEdge_ValType get_wasm_type() = delete;

  template <>
  WasmEdge_ValType get_wasm_type<int32_t>() {
    return WasmEdge_ValTypeGenI32();
  }

  template <>
  WasmEdge_ValType get_wasm_type<uint32_t>() {
    return WasmEdge_ValType_I32;
  }

  template <>
  WasmEdge_ValType get_wasm_type<int64_t>() {
    return WasmEdge_ValTypeGenI64();
  }

  template <>
  WasmEdge_ValType get_wasm_type<uint64_t>() {
    return WasmEdge_ValType_I64;
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

  WasmEdge_Result stub(void *data,
                       const WasmEdge_CallingFrameContext *,
                       const WasmEdge_Value *,
                       WasmEdge_Value *) {
    static log::Logger logger = log::createLogger("WasmEdge", "runtime");
    SL_ERROR(logger,
             "Attempt to call an unimplemented Host method '{}'",
             reinterpret_cast<const char *>(data));
    return WasmEdge_Result_Terminate;
  };

  template <typename Ret, typename... Args>
  void stub_host_method(WasmEdge_ModuleInstanceContext *module,
                        const char *name) {
    register_method<Ret, Args...>(stub, module, const_cast<char *>(name), name);
  }

#define REGISTER_HOST_METHOD(Ret, name, ...)            \
  register_host_method<&host_api::HostApi::name,        \
                       Ret __VA_OPT__(, ) __VA_ARGS__>( \
      instance, host_api, #name);

#define STUB_HOST_METHOD(Ret, name, ...) \
  stub_host_method<Ret __VA_OPT__(, ) __VA_ARGS__>(instance, #name);

  void register_host_api(host_api::HostApi &host_api,
                         WasmEdge_ModuleInstanceContext *instance) {
    BOOST_ASSERT(instance);
    // clang-format off
    REGISTER_HOST_METHOD(void, ext_allocator_free_version_1, WasmPointer)
    REGISTER_HOST_METHOD(void, ext_crypto_start_batch_verify_version_1)
    REGISTER_HOST_METHOD(void, ext_default_child_storage_clear_version_1, WasmSpan, WasmSpan)
    REGISTER_HOST_METHOD(void, ext_default_child_storage_clear_prefix_version_1, WasmSpan, WasmSpan)
    REGISTER_HOST_METHOD(int64_t, ext_default_child_storage_clear_prefix_version_2, int64_t, int64_t, int64_t)
    REGISTER_HOST_METHOD(void, ext_default_child_storage_set_version_1, int64_t, int64_t, int64_t)
    REGISTER_HOST_METHOD(void, ext_default_child_storage_storage_kill_version_1, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_default_child_storage_storage_kill_version_3, int64_t, int64_t)
    REGISTER_HOST_METHOD(void, ext_logging_log_version_1, int32_t, int64_t, int64_t)
    REGISTER_HOST_METHOD(void, ext_misc_print_hex_version_1, int64_t)
    REGISTER_HOST_METHOD(void, ext_misc_print_num_version_1, int64_t)
    REGISTER_HOST_METHOD(void, ext_misc_print_utf8_version_1, int64_t)
    STUB_HOST_METHOD(void, ext_sandbox_instance_teardown_version_1, int32_t)
    STUB_HOST_METHOD(void, ext_sandbox_memory_teardown_version_1, int32_t)
    REGISTER_HOST_METHOD(void, ext_storage_append_version_1, int64_t, int64_t)
    REGISTER_HOST_METHOD(void, ext_storage_clear_prefix_version_1, int64_t)
    REGISTER_HOST_METHOD(void, ext_storage_clear_version_1, int64_t)
    REGISTER_HOST_METHOD(void, ext_storage_commit_transaction_version_1)
    REGISTER_HOST_METHOD(void, ext_storage_rollback_transaction_version_1)
    REGISTER_HOST_METHOD(void, ext_storage_set_version_1, int64_t, int64_t)
    REGISTER_HOST_METHOD(void, ext_storage_start_transaction_version_1)
    REGISTER_HOST_METHOD(int32_t, ext_allocator_malloc_version_1, int32_t)
    REGISTER_HOST_METHOD(int32_t, ext_crypto_ed25519_generate_version_1, int32_t, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_crypto_ed25519_verify_version_1, int32_t, int64_t, int32_t)
    REGISTER_HOST_METHOD(int32_t, ext_crypto_finish_batch_verify_version_1)
    REGISTER_HOST_METHOD(int32_t, ext_crypto_sr25519_generate_version_1, int32_t, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_crypto_sr25519_verify_version_1, int32_t, int64_t, int32_t)
    REGISTER_HOST_METHOD(int32_t, ext_crypto_sr25519_verify_version_2, int32_t, int64_t, int32_t)
    REGISTER_HOST_METHOD(int64_t, ext_crypto_ecdsa_public_keys_version_1, int32_t)
    REGISTER_HOST_METHOD(int64_t, ext_crypto_ecdsa_sign_version_1, int32_t, int32_t, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_crypto_ecdsa_sign_prehashed_version_1, int32_t, int32_t, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_crypto_ecdsa_generate_version_1, int32_t, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_crypto_ecdsa_verify_version_1, int32_t, int64_t, int32_t)
    REGISTER_HOST_METHOD(int32_t, ext_crypto_ecdsa_verify_prehashed_version_1, int32_t, int32_t, int32_t)
    REGISTER_HOST_METHOD(int32_t, ext_crypto_ecdsa_verify_version_2, int32_t, int64_t, int32_t)
    REGISTER_HOST_METHOD(int32_t, ext_default_child_storage_exists_version_1, int64_t, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_hashing_blake2_128_version_1, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_hashing_blake2_256_version_1, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_hashing_keccak_256_version_1, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_hashing_sha2_256_version_1, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_hashing_twox_64_version_1, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_hashing_twox_128_version_1, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_hashing_twox_256_version_1, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_logging_max_level_version_1)
    STUB_HOST_METHOD(int32_t, ext_sandbox_instantiate_version_1, int32_t, int64_t, int64_t, int32_t)
    STUB_HOST_METHOD(int32_t, ext_sandbox_invoke_version_1, int32_t, int64_t, int64_t, int32_t, int32_t, int32_t)
    STUB_HOST_METHOD(int32_t, ext_sandbox_memory_get_version_1, int32_t, int32_t, int32_t, int32_t)
    STUB_HOST_METHOD(int32_t, ext_sandbox_memory_new_version_1, int32_t, int32_t)
    STUB_HOST_METHOD(int32_t, ext_sandbox_memory_set_version_1, int32_t, int32_t, int32_t, int32_t)
    REGISTER_HOST_METHOD(int32_t, ext_storage_exists_version_1, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_trie_blake2_256_ordered_root_version_1, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_trie_blake2_256_ordered_root_version_2, int64_t, int32_t)
    REGISTER_HOST_METHOD(int32_t, ext_trie_keccak_256_ordered_root_version_2, int64_t, int32_t)
    REGISTER_HOST_METHOD(int32_t, ext_trie_blake2_256_root_version_1, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_crypto_ed25519_public_keys_version_1, int32_t)
    REGISTER_HOST_METHOD(int64_t, ext_crypto_ed25519_sign_version_1, int32_t, int32_t, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_crypto_secp256k1_ecdsa_recover_compressed_version_1, int32_t, int32_t)
    REGISTER_HOST_METHOD(int64_t, ext_crypto_secp256k1_ecdsa_recover_compressed_version_2, int32_t, int32_t)
    REGISTER_HOST_METHOD(int64_t, ext_crypto_secp256k1_ecdsa_recover_version_1, int32_t, int32_t)
    REGISTER_HOST_METHOD(int64_t, ext_crypto_secp256k1_ecdsa_recover_version_2, int32_t, int32_t)
    REGISTER_HOST_METHOD(int64_t, ext_crypto_sr25519_public_keys_version_1, int32_t)
    REGISTER_HOST_METHOD(int64_t, ext_crypto_sr25519_sign_version_1, int32_t, int32_t, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_default_child_storage_get_version_1, int64_t, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_default_child_storage_next_key_version_1, int64_t, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_default_child_storage_read_version_1, int64_t, int64_t, int64_t, int32_t)
    REGISTER_HOST_METHOD(int64_t, ext_default_child_storage_root_version_1, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_default_child_storage_root_version_2, int64_t, int32_t)
    REGISTER_HOST_METHOD(int64_t, ext_misc_runtime_version_version_1, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_storage_changes_root_version_1, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_storage_clear_prefix_version_2, int64_t, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_storage_get_version_1,  int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_storage_next_key_version_1, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_storage_read_version_1, int64_t, int64_t, int32_t)
    REGISTER_HOST_METHOD(int64_t, ext_storage_root_version_1)
    REGISTER_HOST_METHOD(int64_t, ext_storage_root_version_2, int32_t)

    // -------------------------- Offchain extension ---------------------------
    REGISTER_HOST_METHOD(int32_t, ext_offchain_is_validator_version_1)
    REGISTER_HOST_METHOD(int64_t, ext_offchain_submit_transaction_version_1, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_offchain_network_state_version_1)
    REGISTER_HOST_METHOD(int64_t, ext_offchain_timestamp_version_1)
    REGISTER_HOST_METHOD(void   , ext_offchain_sleep_until_version_1, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_offchain_random_seed_version_1)
    REGISTER_HOST_METHOD(void   , ext_offchain_local_storage_set_version_1, int32_t, int64_t, int64_t)
    REGISTER_HOST_METHOD(void   , ext_offchain_local_storage_clear_version_1, int32_t, int64_t)
    REGISTER_HOST_METHOD(int32_t, ext_offchain_local_storage_compare_and_set_version_1, int32_t, int64_t, int64_t, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_offchain_local_storage_get_version_1, int32_t, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_offchain_http_request_start_version_1, int64_t, int64_t, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_offchain_http_request_add_header_version_1, int32_t, int64_t, int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_offchain_http_request_write_body_version_1, int32_t,int64_t,int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_offchain_http_response_wait_version_1,int64_t,int64_t)
    REGISTER_HOST_METHOD(int64_t, ext_offchain_http_response_headers_version_1, int32_t)
    REGISTER_HOST_METHOD(int64_t, ext_offchain_http_response_read_body_version_1, int32_t, int64_t, int64_t)
    REGISTER_HOST_METHOD(void   , ext_offchain_set_authorized_nodes_version_1, int64_t, int32_t)
    REGISTER_HOST_METHOD(void   , ext_offchain_index_set_version_1, int64_t, int64_t)
    REGISTER_HOST_METHOD(void   , ext_offchain_index_clear_version_1, int64_t)

    REGISTER_HOST_METHOD(void, ext_panic_handler_abort_on_panic_version_1, int64_t)

    // clang-format on
  }

}  // namespace kagome::runtime::wasm_edge
