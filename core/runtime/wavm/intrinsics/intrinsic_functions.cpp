/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"

#include "runtime/wavm/intrinsics/intrinsic_module.hpp"

namespace kagome::runtime::wavm {

  log::Logger logger;

  static thread_local std::stack<std::shared_ptr<host_api::HostApi>>
      global_host_apis;

  void pushHostApi(std::shared_ptr<host_api::HostApi> api) {
    global_host_apis.emplace(std::move(api));
  }

  void popHostApi() {
    BOOST_ASSERT(!global_host_apis.empty());
    global_host_apis.pop();
  }

  std::shared_ptr<host_api::HostApi> peekHostApi() {
    BOOST_ASSERT(!global_host_apis.empty());
    return global_host_apis.top();
  }

  template <typename T>
  WAVM::IR::ValueType valType() {
    return WAVM::IR::ValueType::v128;
  }

  template <>
  WAVM::IR::ValueType valType<int32_t>() {
    return WAVM::IR::ValueType::i32;
  }

  template <>
  WAVM::IR::ValueType valType<uint32_t>() {
    return WAVM::IR::ValueType::i32;
  }

  template <>
  WAVM::IR::ValueType valType<int64_t>() {
    return WAVM::IR::ValueType::i64;
  }

  template <>
  WAVM::IR::ValueType valType<uint64_t>() {
    return WAVM::IR::ValueType::i64;
  }

  template <typename T, auto f, typename... Args>
  auto function(WAVM::Runtime::ContextRuntimeData *, Args... args) {
    return (peekHostApi().get()->*f)(args...);
  }

  template <typename T, T>
  struct HostApiFunc;
#define HOST_API_FUNC(...)                                            \
  template <typename T,                                               \
            typename R,                                               \
            typename... Args,                                         \
            R (T::*mf)(Args...) __VA_ARGS__>                          \
  struct HostApiFunc<R (T::*)(Args...) __VA_ARGS__, mf> {             \
    using Ret = R;                                                    \
    static WAVM::IR::FunctionType type() {                            \
      WAVM::IR::TypeTuple types({valType<Args>()...});                \
      if constexpr (std::is_same_v<R, void>) {                        \
        return WAVM::IR::FunctionType{{}, types};                     \
      } else {                                                        \
        return WAVM::IR::FunctionType{{valType<R>()}, types};         \
      }                                                               \
    }                                                                 \
    static R (*fun())(WAVM::Runtime::ContextRuntimeData *, Args...) { \
      return function<T, mf, Args...>;                                \
    }                                                                 \
  }

  HOST_API_FUNC();
  HOST_API_FUNC(const);

#define WAVM_DEFINE_INTRINSIC_FUNCTION_STUB(Result, cName, ...)            \
  Result cName(WAVM::Runtime::ContextRuntimeData *contextRuntimeData,      \
               ##__VA_ARGS__) {                                            \
    logger->warn("Unimplemented Host API function " #cName " was called"); \
    return Result();                                                       \
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION_STUB(void,
                                      ext_sandbox_instance_teardown_version_1,
                                      WAVM::I32)

  WAVM_DEFINE_INTRINSIC_FUNCTION_STUB(WAVM::I32,
                                      ext_sandbox_instantiate_version_1,
                                      WAVM::I32,
                                      WAVM::I64,
                                      WAVM::I64,
                                      WAVM::I32)

  WAVM_DEFINE_INTRINSIC_FUNCTION_STUB(WAVM::I32,
                                      ext_sandbox_invoke_version_1,
                                      WAVM::I32,
                                      WAVM::I64,
                                      WAVM::I64,
                                      WAVM::I32,
                                      WAVM::I32,
                                      WAVM::I32)

  WAVM_DEFINE_INTRINSIC_FUNCTION_STUB(WAVM::I32,
                                      ext_sandbox_memory_get_version_1,
                                      WAVM::I32,
                                      WAVM::I32,
                                      WAVM::I32,
                                      WAVM::I32)

  WAVM_DEFINE_INTRINSIC_FUNCTION_STUB(WAVM::I32,
                                      ext_sandbox_memory_new_version_1,
                                      WAVM::I32,
                                      WAVM::I32)

  WAVM_DEFINE_INTRINSIC_FUNCTION_STUB(WAVM::I32,
                                      ext_sandbox_memory_set_version_1,
                                      WAVM::I32,
                                      WAVM::I32,
                                      WAVM::I32,
                                      WAVM::I32)

  WAVM_DEFINE_INTRINSIC_FUNCTION_STUB(void,
                                      ext_sandbox_memory_teardown_version_1,
                                      WAVM::I32)

  template <auto mf>
  void registerHostIntrinsic(const std::string &name, IntrinsicModule &module) {
    static const std::string fname = name;
    module.addFunction(fname,
                       HostApiFunc<decltype(mf), mf>::fun(),
                       HostApiFunc<decltype(mf), mf>::type());
    if (logger == nullptr) {
      logger = log::createLogger("Host API wrappers", "wavm");
    }
    logger->trace("registered {}", fname);
  }

#define REGISTER_HOST_INTRINSIC(name) \
  registerHostIntrinsic<&host_api::HostApi ::name>(#name, module)

  void registerHostApiMethods(IntrinsicModule &module) {
    if (logger == nullptr) {
      logger = log::createLogger("Host API wrappers", "wavm");
    }

#define REGISTER_HOST_INTRINSIC_STUB(Ret, name, ...) \
  module.addFunction(                                \
      #name, &name, WAVM::IR::FunctionType{{Ret}, {__VA_ARGS__}});

    auto I32 = WAVM::IR::ValueType::i32;
    auto I64 = WAVM::IR::ValueType::i64;
    // clang-format off
    REGISTER_HOST_INTRINSIC(ext_allocator_free_version_1);
    REGISTER_HOST_INTRINSIC(ext_allocator_malloc_version_1);
    REGISTER_HOST_INTRINSIC(ext_crypto_ed25519_generate_version_1);
    REGISTER_HOST_INTRINSIC(ext_crypto_ed25519_public_keys_version_1);
    REGISTER_HOST_INTRINSIC(ext_crypto_ed25519_sign_version_1);
    REGISTER_HOST_INTRINSIC(ext_crypto_ed25519_verify_version_1);
    REGISTER_HOST_INTRINSIC(ext_crypto_finish_batch_verify_version_1);
    REGISTER_HOST_INTRINSIC(ext_crypto_secp256k1_ecdsa_recover_compressed_version_1);
    REGISTER_HOST_INTRINSIC(ext_crypto_secp256k1_ecdsa_recover_compressed_version_2);
    REGISTER_HOST_INTRINSIC(ext_crypto_secp256k1_ecdsa_recover_version_1);
    REGISTER_HOST_INTRINSIC(ext_crypto_secp256k1_ecdsa_recover_version_2);
    REGISTER_HOST_INTRINSIC(ext_crypto_sr25519_generate_version_1);
    REGISTER_HOST_INTRINSIC(ext_crypto_sr25519_public_keys_version_1);
    REGISTER_HOST_INTRINSIC(ext_crypto_sr25519_sign_version_1);
    REGISTER_HOST_INTRINSIC(ext_crypto_sr25519_verify_version_1);
    REGISTER_HOST_INTRINSIC(ext_crypto_sr25519_verify_version_2);
    REGISTER_HOST_INTRINSIC(ext_crypto_start_batch_verify_version_1);
    REGISTER_HOST_INTRINSIC(ext_default_child_storage_clear_version_1);
    REGISTER_HOST_INTRINSIC(ext_default_child_storage_clear_prefix_version_1);
    REGISTER_HOST_INTRINSIC(ext_default_child_storage_exists_version_1);
    REGISTER_HOST_INTRINSIC(ext_default_child_storage_get_version_1);
    REGISTER_HOST_INTRINSIC(ext_default_child_storage_storage_kill_version_1);
    REGISTER_HOST_INTRINSIC(ext_default_child_storage_next_key_version_1);
    REGISTER_HOST_INTRINSIC(ext_default_child_storage_read_version_1);
    REGISTER_HOST_INTRINSIC(ext_default_child_storage_root_version_1);
    REGISTER_HOST_INTRINSIC(ext_default_child_storage_set_version_1);
    REGISTER_HOST_INTRINSIC(ext_hashing_blake2_128_version_1);
    REGISTER_HOST_INTRINSIC(ext_hashing_blake2_256_version_1);
    REGISTER_HOST_INTRINSIC(ext_hashing_keccak_256_version_1);
    REGISTER_HOST_INTRINSIC(ext_hashing_sha2_256_version_1);
    REGISTER_HOST_INTRINSIC(ext_hashing_twox_128_version_1);
    REGISTER_HOST_INTRINSIC(ext_hashing_twox_256_version_1);
    REGISTER_HOST_INTRINSIC(ext_hashing_twox_64_version_1);
    REGISTER_HOST_INTRINSIC(ext_logging_log_version_1);
    REGISTER_HOST_INTRINSIC(ext_logging_max_level_version_1);
    REGISTER_HOST_INTRINSIC(ext_misc_print_hex_version_1);
    REGISTER_HOST_INTRINSIC(ext_misc_print_num_version_1);
    REGISTER_HOST_INTRINSIC(ext_misc_print_utf8_version_1);
    REGISTER_HOST_INTRINSIC(ext_misc_runtime_version_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_http_request_add_header_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_http_request_start_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_http_request_write_body_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_http_response_headers_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_http_response_read_body_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_http_response_wait_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_index_clear_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_index_set_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_is_validator_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_local_storage_clear_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_local_storage_compare_and_set_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_local_storage_get_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_local_storage_set_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_network_state_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_random_seed_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_set_authorized_nodes_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_sleep_until_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_submit_transaction_version_1);
    REGISTER_HOST_INTRINSIC(ext_offchain_timestamp_version_1);
    REGISTER_HOST_INTRINSIC(ext_storage_append_version_1);
    REGISTER_HOST_INTRINSIC(ext_storage_changes_root_version_1);
    REGISTER_HOST_INTRINSIC(ext_storage_clear_prefix_version_1);
    REGISTER_HOST_INTRINSIC(ext_storage_clear_prefix_version_2);
    REGISTER_HOST_INTRINSIC(ext_storage_clear_version_1);
    REGISTER_HOST_INTRINSIC(ext_storage_commit_transaction_version_1);
    REGISTER_HOST_INTRINSIC(ext_storage_exists_version_1);
    REGISTER_HOST_INTRINSIC(ext_storage_get_version_1);
    REGISTER_HOST_INTRINSIC(ext_storage_next_key_version_1);
    REGISTER_HOST_INTRINSIC(ext_storage_read_version_1);
    REGISTER_HOST_INTRINSIC(ext_storage_rollback_transaction_version_1);
    REGISTER_HOST_INTRINSIC(ext_storage_root_version_1);
    REGISTER_HOST_INTRINSIC(ext_storage_set_version_1);
    REGISTER_HOST_INTRINSIC(ext_storage_start_transaction_version_1);
    REGISTER_HOST_INTRINSIC(ext_trie_blake2_256_ordered_root_version_1);
    REGISTER_HOST_INTRINSIC(ext_trie_blake2_256_root_version_1);
    REGISTER_HOST_INTRINSIC_STUB(, ext_sandbox_instance_teardown_version_1, I32)
    REGISTER_HOST_INTRINSIC_STUB(, ext_sandbox_memory_teardown_version_1, I32)
    REGISTER_HOST_INTRINSIC_STUB(I32, ext_sandbox_instantiate_version_1, I32, I64, I64, I32)
    REGISTER_HOST_INTRINSIC_STUB(I32, ext_sandbox_invoke_version_1, I32, I64, I64, I32, I32, I32)
    REGISTER_HOST_INTRINSIC_STUB(I32, ext_sandbox_memory_get_version_1, I32, I32, I32, I32)
    REGISTER_HOST_INTRINSIC_STUB(I32, ext_sandbox_memory_new_version_1, I32, I32)
    REGISTER_HOST_INTRINSIC_STUB(I32, ext_sandbox_memory_set_version_1, I32, I32, I32, I32)
    // clang-format on
  }

}  // namespace kagome::runtime::wavm
