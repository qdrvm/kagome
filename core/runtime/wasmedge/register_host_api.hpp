#pragma once

#include <wasmedge.h>

#include <string>
#include <tuple>
#include <utility>

namespace {
  template <typename T>
  struct MakeSigned {
    using type = typename std::
        conditional_t<std::is_integral_v<T>, typename std::make_signed_t<T>, T>;
  };
  template <>
  struct MakeSigned<void> {
    using type = void;
  };

  template <unsigned N>
  struct CommonInt {
    using type = __int128;
  };

  template <>
  struct CommonInt<4> {
    using type = int32_t;
  };

  template <>
  struct CommonInt<8> {
    using type = int64_t;
  };

  template <typename T, typename Enable = void>
  struct MakeCommon;

  template <typename T>
  struct MakeCommon<T, typename std::enable_if_t<std::is_integral_v<T>>> {
    using type = typename CommonInt<sizeof(T)>::type;
  };

  template <typename T>
  struct MakeCommon<T, typename std::enable_if_t<not std::is_integral_v<T>>> {
    using type = T;
  };

  template <typename T>
  WasmEdge_Value (*fromType())(const T) {
    return nullptr;
  }
  template <>
  WasmEdge_Value (*fromType<int32_t>())(const int32_t) {
    return WasmEdge_ValueGenI32;
  }
  template <>
  WasmEdge_Value (*fromType<int64_t>())(const int64_t) {
    return WasmEdge_ValueGenI64;
  }
  template <>
  WasmEdge_Value (*fromType<float>())(const float) {
    return WasmEdge_ValueGenF32;
  }
  template <>
  WasmEdge_Value (*fromType<double>())(const double) {
    return WasmEdge_ValueGenF64;
  }
  template <>
  WasmEdge_Value (*fromType<__int128>())(const __int128) {
    return WasmEdge_ValueGenV128;
  }

  template <typename T>
  T (*toType())
  (const WasmEdge_Value) {
    return nullptr;
  }
  template <>
  int32_t (*toType<int32_t>())(const WasmEdge_Value) {
    return WasmEdge_ValueGetI32;
  }
  template <>
  int64_t (*toType<int64_t>())(const WasmEdge_Value) {
    return WasmEdge_ValueGetI64;
  }
  template <>
  float (*toType<float>())(const WasmEdge_Value) {
    return WasmEdge_ValueGetF32;
  }
  template <>
  double (*toType<double>())(const WasmEdge_Value) {
    return WasmEdge_ValueGetF64;
  }
  template <>
  __int128 (*toType<__int128>())(const WasmEdge_Value) {
    return WasmEdge_ValueGetV128;
  }

  template <typename T>
  WasmEdge_ValType type() {
    return WasmEdge_ValType_V128;
  }
  template <>
  WasmEdge_ValType type<int32_t>() {
    return WasmEdge_ValType_I32;
  }
  template <>
  WasmEdge_ValType type<int64_t>() {
    return WasmEdge_ValType_I64;
  }
  template <>
  WasmEdge_ValType type<float>() {
    return WasmEdge_ValType_F32;
  }
  template <>
  WasmEdge_ValType type<double>() {
    return WasmEdge_ValType_F64;
  }

  template <typename T, auto f, typename TP, size_t... I>
  auto internal_fun(T *fun,
                    const WasmEdge_Value *In,
                    const TP &tup,
                    std::index_sequence<I...>) {
    return (fun->*f)(std::get<I>(tup)(In[I])...);
  }

  template <typename T, T>
  struct HostApiFuncRet;
  template <typename T, typename R, typename... Args, R (T::*mf)(Args...)>
  struct HostApiFuncRet<R (T::*)(Args...), mf> {
    using Ret = R;
  };
  template <typename T, typename R, typename... Args, R (T::*mf)(Args...) const>
  struct HostApiFuncRet<R (T::*)(Args...) const, mf> {
    using Ret = R;
  };

  /* Host function body definition. */
  template <typename T, auto f, typename... Args>
  WasmEdge_Result fun(void *Data,
                      WasmEdge_MemoryInstanceContext *MemCxt,
                      const WasmEdge_Value *In,
                      WasmEdge_Value *Out) {
    auto indices = std::index_sequence_for<Args...>{};
    auto tup = std::make_tuple(toType<typename MakeSigned<Args>::type>()...);
    if constexpr (not std::is_same_v<
                      void,
                      typename HostApiFuncRet<decltype(f), f>::Ret>) {
      auto res = internal_fun<T, f>(static_cast<T *>(Data), In, tup, indices);
      Out[0] = fromType<typename MakeCommon<decltype(res)>::type>()(res);
    } else {
      internal_fun<T, f>(static_cast<T *>(Data), In, tup, indices);
    }
    return WasmEdge_Result_Success;
  }

  template <typename T, T>
  struct HostApiFunc;
  template <typename T, typename R, typename... Args, R (T::*mf)(Args...)>
  struct HostApiFunc<R (T::*)(Args...), mf> {
    using Ret = R;
    static WasmEdge_Result (*fun())(void *Data,
                                    WasmEdge_MemoryInstanceContext *MemCxt,
                                    const WasmEdge_Value *In,
                                    WasmEdge_Value *Out) {
      return ::fun<T, mf, Args...>;
    }

    static WasmEdge_FunctionTypeContext *type() {
      enum WasmEdge_ValType ParamList[sizeof...(Args)] = {
          ::type<typename MakeSigned<Args>::type>()...};

      size_t r = 1;
      enum WasmEdge_ValType ReturnListNonVoid[1] = {
          ::type<typename MakeSigned<R>::type>()};
      enum WasmEdge_ValType *ReturnList = ReturnListNonVoid;

      if constexpr (std::is_same_v<R, void>) {
        r = 0;
        ReturnList = nullptr;
      }

      return WasmEdge_FunctionTypeCreate(
          ParamList, sizeof...(Args), ReturnList, r);
    }
  };

  template <typename T, typename R, typename... Args, R (T::*mf)(Args...) const>
  struct HostApiFunc<R (T::*)(Args...) const, mf> {
    using Ret = R;
    static WasmEdge_Result (*fun())(void *Data,
                                    WasmEdge_MemoryInstanceContext *MemCxt,
                                    const WasmEdge_Value *In,
                                    WasmEdge_Value *Out) {
      return ::fun<T, mf, Args...>;
    }

    static WasmEdge_FunctionTypeContext *type() {
      enum WasmEdge_ValType ParamList[sizeof...(Args)] = {
          ::type<typename MakeSigned<Args>::type>()...};

      size_t r = 1;
      enum WasmEdge_ValType ReturnListNonVoid[1] = {
          ::type<typename MakeSigned<R>::type>()};
      enum WasmEdge_ValType *ReturnList = ReturnListNonVoid;

      if constexpr (std::is_same_v<R, void>) {
        r = 0;
        ReturnList = nullptr;
      }

      return WasmEdge_FunctionTypeCreate(
          ParamList, sizeof...(Args), ReturnList, r);
    }
  };
}  // namespace

template <auto mf>
void registerHostApiFunc(const std::string &name,
                         WasmEdge_ImportObjectContext *ImpObj) {
  WasmEdge_FunctionTypeContext *HostFType =
      HostApiFunc<decltype(mf), mf>::type();
  WasmEdge_HostFunctionContext *HostFunc = WasmEdge_HostFunctionCreate(
      HostFType, HostApiFunc<decltype(mf), mf>::fun(), 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  WasmEdge_String HostFuncName = WasmEdge_StringCreateByCString(name.c_str());
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostFuncName, HostFunc);
  WasmEdge_StringDelete(HostFuncName);
}

#define REGISTER_HOST_API_FUNC(class, name, obj) \
  registerHostApiFunc<&class ::name>(#name, obj)

namespace {
  using kagome::host_api::HostApi;
  using kagome::host_api::HostApiImpl;
}  // namespace

inline void register_host_api(WasmEdge_ImportObjectContext *ImpObj) {
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_trie_blake2_256_ordered_root_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_offchain_index_set_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_logging_log_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_crypto_ed25519_generate_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_crypto_ed25519_verify_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_crypto_finish_batch_verify_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_crypto_secp256k1_ecdsa_recover_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApi, ext_crypto_secp256k1_ecdsa_recover_compressed_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_crypto_sr25519_generate_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_crypto_sr25519_public_keys_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_crypto_sr25519_sign_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_crypto_sr25519_verify_version_2, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_crypto_start_batch_verify_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_hashing_blake2_128_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_hashing_blake2_256_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_hashing_keccak_256_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_hashing_twox_128_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_hashing_twox_64_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_allocator_free_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_allocator_malloc_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_misc_print_hex_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_misc_print_num_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_misc_print_utf8_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_misc_runtime_version_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_offchain_is_validator_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_offchain_local_storage_clear_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl,
                         ext_offchain_local_storage_compare_and_set_version_1,
                         ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_offchain_local_storage_get_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_offchain_local_storage_set_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_offchain_network_state_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_offchain_random_seed_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_offchain_submit_transaction_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_offchain_timestamp_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_storage_append_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_storage_changes_root_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_storage_clear_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_storage_clear_prefix_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_storage_clear_prefix_version_2, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_storage_commit_transaction_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_storage_exists_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_storage_get_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_storage_next_key_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_storage_read_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_storage_rollback_transaction_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_storage_root_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(HostApiImpl, ext_storage_set_version_1, ImpObj);
  REGISTER_HOST_API_FUNC(
      HostApiImpl, ext_storage_start_transaction_version_1, ImpObj);
}
