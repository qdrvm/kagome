/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/impl/intrinsic_functions.hpp"

#include "runtime/wavm/intrinsic_resolver.hpp"

namespace kagome::runtime::wavm {

  log::Logger logger;

  WAVM::Intrinsics::Module *getIntrinsicModule_env() {
    static WAVM::Intrinsics::Module module;
    return &module;
  }

  std::stack<std::shared_ptr<host_api::HostApi>>
      global_host_apis;  //< TODO(Harrm) fix this wild crutch

  void pushHostApi(std::shared_ptr<host_api::HostApi> api) {
    global_host_apis.emplace(std::move(api));
  }

  void popHostApi() {
    global_host_apis.pop();
  }

  std::shared_ptr<host_api::HostApi> peekHostApi() {
    return global_host_apis.top();
  }

  const WAVM::IR::MemoryType kIntrinsicMemoryType{
      WAVM::IR::MemoryType(false, WAVM::IR::IndexType::i32, {20, UINT64_MAX})};

  // actually used, because added to the intrinsic module
  [[maybe_unused]] WAVM::Intrinsics::Memory env_memory{
      getIntrinsicModule_env(),
      kIntrinsicMemoryName.data(),
      kIntrinsicMemoryType};

  void registerHostApiMethods(IntrinsicResolver &resolver,
                              const host_api::HostApi &host_api) {
#define GENERATE_HOST_INTRINSIC(Ret, name, ...) \
  resolver.addIntrinsic(#name, &kagome::runtime::wavm::name##Intrinsic)

#define GENERATE_STUB_INTRINSIC(Ret, name, ...) \
  resolver.addIntrinsic(#name, &kagome::runtime::wavm::name##Intrinsic)

    // clang-format off
    GENERATE_HOST_INTRINSIC(void, ext_logging_log_version_1, WAVM::I32, WAVM::I64, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I32, ext_hashing_twox_128_version_1, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I32,ext_hashing_twox_64_version_1,  WAVM::I64);
    GENERATE_HOST_INTRINSIC(void, ext_storage_set_version_1, WAVM::I64, WAVM::I64);
    GENERATE_HOST_INTRINSIC(void, ext_storage_clear_version_1, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I32,ext_hashing_blake2_128_version_1, WAVM::I64);
    GENERATE_HOST_INTRINSIC(void, ext_storage_clear_prefix_version_1, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I64,ext_storage_get_version_1,  WAVM::I64);
    GENERATE_HOST_INTRINSIC(void, ext_misc_print_utf8_version_1, WAVM::I64);
    GENERATE_STUB_INTRINSIC(WAVM::I32, ext_offchain_random_seed_version_1);
    GENERATE_HOST_INTRINSIC(void, ext_misc_print_hex_version_1, WAVM::I64);
    GENERATE_HOST_INTRINSIC(void, ext_crypto_start_batch_verify_version_1);
    GENERATE_HOST_INTRINSIC(WAVM::I32, ext_crypto_finish_batch_verify_version_1);
    GENERATE_STUB_INTRINSIC(WAVM::I32, ext_offchain_is_validator_version_1);
    GENERATE_STUB_INTRINSIC(WAVM::I64, ext_offchain_local_storage_get_version_1, WAVM::I32, WAVM::I64);
    GENERATE_STUB_INTRINSIC(WAVM::I32, ext_offchain_local_storage_compare_and_set_version_1, WAVM::I32, WAVM::I64, WAVM::I64, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I32, ext_hashing_blake2_256_version_1, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I32, ext_hashing_keccak_256_version_1, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I32, ext_crypto_ed25519_verify_version_1, WAVM::I32, WAVM::I64, WAVM::I32);
    GENERATE_HOST_INTRINSIC(WAVM::I64, ext_misc_runtime_version_version_1, WAVM::I64);
    GENERATE_HOST_INTRINSIC(void, ext_storage_append_version_1, WAVM::I64, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I64, ext_storage_next_key_version_1, WAVM::I64);
    GENERATE_HOST_INTRINSIC(void, ext_misc_print_num_version_1, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I32, ext_crypto_sr25519_verify_version_2, WAVM::I32, WAVM::I64, WAVM::I32);
    GENERATE_STUB_INTRINSIC(void, ext_offchain_local_storage_set_version_1, WAVM::I32, WAVM::I64, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I64, ext_storage_root_version_1);
    GENERATE_HOST_INTRINSIC(WAVM::I64, ext_storage_changes_root_version_1, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I32, ext_trie_blake2_256_ordered_root_version_1, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I32, ext_crypto_ed25519_generate_version_1, WAVM::I32, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I64, ext_crypto_secp256k1_ecdsa_recover_version_1, WAVM::I32, WAVM::I32);
    GENERATE_HOST_INTRINSIC(WAVM::I64, ext_crypto_secp256k1_ecdsa_recover_compressed_version_1, WAVM::I32, WAVM::I32);
    GENERATE_HOST_INTRINSIC(WAVM::I32, ext_crypto_sr25519_generate_version_1, WAVM::I32, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I64, ext_crypto_sr25519_public_keys_version_1, WAVM::I32);
    GENERATE_HOST_INTRINSIC(WAVM::I64, ext_crypto_sr25519_sign_version_1, WAVM::I32, WAVM::I32, WAVM::I64);
    GENERATE_STUB_INTRINSIC(WAVM::I64, ext_offchain_network_state_version_1);
    GENERATE_STUB_INTRINSIC(WAVM::I64, ext_offchain_submit_transaction_version_1, WAVM::I64);
    GENERATE_HOST_INTRINSIC(WAVM::I64, ext_storage_read_version_1, WAVM::I64, WAVM::I64, WAVM::I32);
    GENERATE_HOST_INTRINSIC(WAVM::I32, ext_allocator_malloc_version_1, WAVM::I32);
    GENERATE_HOST_INTRINSIC(void, ext_allocator_free_version_1, WAVM::I32);
    // clang-format on
  }

#undef WAVM_DEFINE_INTRINSIC_FUNCTION
#define WAVM_DEFINE_INTRINSIC_FUNCTION(module, nameString, Result, cName, ...) \
  Result cName(WAVM::Runtime::ContextRuntimeData *contextRuntimeData,          \
               ##__VA_ARGS__);                                                 \
  WAVM::Intrinsics::Function cName##Intrinsic(                                 \
      getIntrinsicModule_##module(),                                           \
      nameString,                                                              \
      (void *)&cName,                                                          \
      WAVM::Intrinsics::inferIntrinsicFunctionType(&cName));                   \
  Result cName(WAVM::Runtime::ContextRuntimeData *contextRuntimeData,          \
               ##__VA_ARGS__)

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_allocator_free_version_1",
                                 void,
                                 ext_allocator_free_version_1,
                                 WAVM::I32 address) {
    return global_host_apis.top()->ext_allocator_free_version_1(address);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_allocator_malloc_version_1",
                                 WAVM::I32,
                                 ext_allocator_malloc_version_1,
                                 WAVM::I32 size) {
    return global_host_apis.top()->ext_allocator_malloc_version_1(size);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_crypto_ed25519_generate_version_1",
                                 WAVM::I32,
                                 ext_crypto_ed25519_generate_version_1,
                                 WAVM::I32 keytype,
                                 WAVM::I64 seed) {
    return global_host_apis.top()->ext_crypto_ed25519_generate_version_1(
        keytype, seed);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_crypto_ed25519_verify_version_1",
                                 WAVM::I32,
                                 ext_crypto_ed25519_verify_version_1,
                                 WAVM::I32 sig_data,
                                 WAVM::I64 msg,
                                 WAVM::I32 pubkey_data) {
    return global_host_apis.top()->ext_crypto_ed25519_verify_version_1(
        sig_data, msg, pubkey_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_crypto_finish_batch_verify_version_1",
                                 WAVM::I32,
                                 ext_crypto_finish_batch_verify_version_1) {
    return global_host_apis.top()->ext_crypto_finish_batch_verify_version_1();
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_crypto_secp256k1_ecdsa_recover_version_1",
                                 WAVM::I64,
                                 ext_crypto_secp256k1_ecdsa_recover_version_1,
                                 WAVM::I32 sig,
                                 WAVM::I32 msg) {
    return global_host_apis.top()->ext_crypto_secp256k1_ecdsa_recover_version_1(
        sig, msg);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(
      env,
      "ext_crypto_secp256k1_ecdsa_recover_compressed_version_1",
      WAVM::I64,
      ext_crypto_secp256k1_ecdsa_recover_compressed_version_1,
      WAVM::I32 sig,
      WAVM::I32 msg) {
    return global_host_apis.top()
        ->ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(sig, msg);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_crypto_sr25519_generate_version_1",
                                 WAVM::I32,
                                 ext_crypto_sr25519_generate_version_1,
                                 WAVM::I32 key_type,
                                 WAVM::I64 seed) {
    return global_host_apis.top()->ext_crypto_sr25519_generate_version_1(
        key_type, seed);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_crypto_sr25519_public_keys_version_1",
                                 WAVM::I64,
                                 ext_crypto_sr25519_public_keys_version_1,
                                 WAVM::I32 key_type) {
    return global_host_apis.top()->ext_crypto_sr25519_public_keys_version_1(
        key_type);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_crypto_sr25519_sign_version_1",
                                 WAVM::I64,
                                 ext_crypto_sr25519_sign_version_1,
                                 WAVM::I32 key_type,
                                 WAVM::I32 key,
                                 WAVM::I64 msg_data) {
    return global_host_apis.top()->ext_crypto_sr25519_sign_version_1(
        key_type, key, msg_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_crypto_sr25519_verify_version_2",
                                 WAVM::I32,
                                 ext_crypto_sr25519_verify_version_2,
                                 WAVM::I32 sig_data,
                                 WAVM::I64 msg,
                                 WAVM::I32 pubkey_data) {
    return global_host_apis.top()->ext_crypto_sr25519_verify_version_2(
        sig_data, msg, pubkey_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_crypto_start_batch_verify_version_1",
                                 void,
                                 ext_crypto_start_batch_verify_version_1) {
    return global_host_apis.top()->ext_crypto_start_batch_verify_version_1();
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_trie_blake2_256_ordered_root_version_1",
                                 WAVM::I32,
                                 ext_trie_blake2_256_ordered_root_version_1,
                                 WAVM::I64 values_data) {
    return global_host_apis.top()->ext_trie_blake2_256_ordered_root_version_1(
        values_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_misc_print_hex_version_1",
                                 void,
                                 ext_misc_print_hex_version_1,
                                 WAVM::I64 values_data) {
    return global_host_apis.top()->ext_misc_print_hex_version_1(values_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_misc_print_num_version_1",
                                 void,
                                 ext_misc_print_num_version_1,
                                 WAVM::I64 values_data) {
    return global_host_apis.top()->ext_misc_print_num_version_1(values_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_misc_print_utf8_version_1",
                                 void,
                                 ext_misc_print_utf8_version_1,
                                 WAVM::I64 values_data) {
    return global_host_apis.top()->ext_misc_print_utf8_version_1(values_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_misc_runtime_version_version_1",
                                 WAVM::I64,
                                 ext_misc_runtime_version_version_1,
                                 WAVM::I64 values_data) {
    return global_host_apis.top()->ext_misc_runtime_version_version_1(
        values_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_default_child_storage_clear_version_1",
                                 void,
                                 ext_default_child_storage_clear_version_1,
                                 WAVM::I64,
                                 WAVM::I64) {
    logger->warn(
        "Unimplemented ext_default_child_storage_clear_version_1 was called");
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_default_child_storage_get_version_1",
                                 WAVM::I64,
                                 ext_default_child_storage_get_version_1,
                                 WAVM::I64,
                                 WAVM::I64) {
    logger->warn(
        "Unimplemented ext_default_child_storage_get_version_1 was called");
    return {};
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_default_child_storage_root_version_1",
                                 WAVM::I64,
                                 ext_default_child_storage_root_version_1,
                                 WAVM::I64) {
    logger->warn(
        "Unimplemented ext_default_child_storage_root_version_1 was called");
    return {};
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_default_child_storage_set_version_1",
                                 void,
                                 ext_default_child_storage_set_version_1,
                                 WAVM::I64,
                                 WAVM::I64,
                                 WAVM::I64) {
    logger->warn(
        "Unimplemented ext_default_child_storage_set_version_1 was called");
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(
      env,
      "ext_default_child_storage_storage_kill_version_1",
      void,
      ext_default_child_storage_storage_kill_version_1,
      WAVM::I64) {
    logger->warn(
        "Unimplemented ext_default_child_storage_storage_kill_version_1 was "
        "called");
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_hashing_blake2_128_version_1",
                                 WAVM::I32,
                                 ext_hashing_blake2_128_version_1,
                                 WAVM::I64 data) {
    return global_host_apis.top()->ext_hashing_blake2_128_version_1(data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_hashing_blake2_256_version_1",
                                 WAVM::I32,
                                 ext_hashing_blake2_256_version_1,
                                 WAVM::I64 data) {
    return global_host_apis.top()->ext_hashing_blake2_256_version_1(data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_hashing_keccak_256_version_1",
                                 WAVM::I32,
                                 ext_hashing_keccak_256_version_1,
                                 WAVM::I64 data) {
    return global_host_apis.top()->ext_hashing_keccak_256_version_1(data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_hashing_sha2_256_version_1",
                                 WAVM::I32,
                                 ext_hashing_sha2_256_version_1,
                                 WAVM::I64 data) {
    return global_host_apis.top()->ext_hashing_sha2_256_version_1(data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_hashing_twox_128_version_1",
                                 WAVM::I32,
                                 ext_hashing_twox_128_version_1,
                                 WAVM::I64 data) {
    return global_host_apis.top()->ext_hashing_twox_128_version_1(data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_hashing_twox_64_version_1",
                                 WAVM::I32,
                                 ext_hashing_twox_64_version_1,
                                 WAVM::I64 data) {
    return global_host_apis.top()->ext_hashing_twox_64_version_1(data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_offchain_is_validator_version_1",
                                 WAVM::I32,
                                 ext_offchain_is_validator_version_1) {
    logger->warn(
        "Unimplemented ext_offchain_is_validator_version_1 was called");
    return {};
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(
      env,
      "ext_offchain_local_storage_compare_and_set_version_1",
      WAVM::I32,
      ext_offchain_local_storage_compare_and_set_version_1,
      WAVM::I32,
      WAVM::I64,
      WAVM::I64,
      WAVM::I64) {
    logger->warn(
        "Unimplemented ext_offchain_local_storage_compare_and_set_version_1 "
        "was called");
    return {};
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_offchain_local_storage_get_version_1",
                                 WAVM::I64,
                                 ext_offchain_local_storage_get_version_1,
                                 WAVM::I32,
                                 WAVM::I64) {
    logger->warn(
        "Unimplemented ext_offchain_local_storage_get_version_1 was called");
    return {};
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_offchain_local_storage_set_version_1",
                                 void,
                                 ext_offchain_local_storage_set_version_1,
                                 WAVM::I32,
                                 WAVM::I64,
                                 WAVM::I64) {
    logger->warn(
        "Unimplemented ext_offchain_local_storage_set_version_1 was called");
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_offchain_network_state_version_1",
                                 WAVM::I64,
                                 ext_offchain_network_state_version_1) {
    logger->warn(
        "Unimplemented ext_offchain_network_state_version_1 was called");
    return {};
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_offchain_random_seed_version_1",
                                 WAVM::I32,
                                 ext_offchain_random_seed_version_1) {
    logger->warn("Unimplemented ext_offchain_random_seed_version_1 was called");
    return {};
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_offchain_submit_transaction_version_1",
                                 WAVM::I64,
                                 ext_offchain_submit_transaction_version_1,
                                 WAVM::I64) {
    logger->warn(
        "Unimplemented ext_offchain_submit_transaction_version_1 was called");
    return {};
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_storage_append_version_1",
                                 void,
                                 ext_storage_append_version_1,
                                 WAVM::I64 key,
                                 WAVM::I64 value) {
    return global_host_apis.top()->ext_storage_append_version_1(key, value);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_storage_changes_root_version_1",
                                 WAVM::I64,
                                 ext_storage_changes_root_version_1,
                                 WAVM::I64 parent_hash) {
    return global_host_apis.top()->ext_storage_changes_root_version_1(
        parent_hash);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_storage_clear_version_1",
                                 void,
                                 ext_storage_clear_version_1,
                                 WAVM::I64 key_data) {
    return global_host_apis.top()->ext_storage_clear_version_1(key_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_storage_clear_prefix_version_1",
                                 void,
                                 ext_storage_clear_prefix_version_1,
                                 WAVM::I64 key_data) {
    return global_host_apis.top()->ext_storage_clear_prefix_version_1(key_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_storage_commit_transaction_version_1",
                                 void,
                                 ext_storage_commit_transaction_version_1) {
    logger->warn(
        "Unimplemented ext_storage_commit_transaction_version_1 was called");
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_storage_get_version_1",
                                 WAVM::I64,
                                 ext_storage_get_version_1,
                                 WAVM::I64 key) {
    return global_host_apis.top()->ext_storage_get_version_1(key);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_storage_next_key_version_1",
                                 WAVM::I64,
                                 ext_storage_next_key_version_1,
                                 WAVM::I64 key) {
    return global_host_apis.top()->ext_storage_next_key_version_1(key);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_storage_read_version_1",
                                 WAVM::I64,
                                 ext_storage_read_version_1,
                                 WAVM::I64 key,
                                 WAVM::I64 value_out,
                                 WAVM::I32 offset) {
    return global_host_apis.top()->ext_storage_read_version_1(
        key, value_out, offset);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_storage_rollback_transaction_version_1",
                                 void,
                                 ext_storage_rollback_transaction_version_1) {
    logger->warn(
        "Unimplemented ext_storage_rollback_transaction_version_1 was called");
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_storage_root_version_1",
                                 WAVM::I64,
                                 ext_storage_root_version_1) {
    return global_host_apis.top()->ext_storage_root_version_1();
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_storage_set_version_1",
                                 void,
                                 ext_storage_set_version_1,
                                 WAVM::I64 key,
                                 WAVM::I64 value) {
    return global_host_apis.top()->ext_storage_set_version_1(key, value);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_storage_start_transaction_version_1",
                                 void,
                                 ext_storage_start_transaction_version_1) {
    logger->warn(
        "Unimplemented ext_storage_start_transaction_version_1 was called");
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_offchain_index_set_version_1",
                                 void,
                                 ext_offchain_index_set_version_1,
                                 WAVM::I64,
                                 WAVM::I64) {
    logger->warn("Unimplemented ext_offchain_index_set_version_1 was called");
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_logging_log_version_1",
                                 void,
                                 ext_logging_log_version_1,
                                 WAVM::I32 level,
                                 WAVM::I64 target,
                                 WAVM::I64 message) {
    return global_host_apis.top()->ext_logging_log_version_1(
        level, target, message);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_sandbox_instance_teardown_version_1",
                                 void,
                                 ext_sandbox_instance_teardown_version_1,
                                 WAVM::I32) {
    logger->warn(
        "Unimplemented ext_sandbox_instance_teardown_version_1 was called");
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_sandbox_instantiate_version_1",
                                 WAVM::I32,
                                 ext_sandbox_instantiate_version_1,
                                 WAVM::I32,
                                 WAVM::I64,
                                 WAVM::I64,
                                 WAVM::I32) {
    logger->warn("Unimplemented ext_sandbox_instantiate_version_1 was called");
    return {};
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_sandbox_invoke_version_1",
                                 WAVM::I32,
                                 ext_sandbox_invoke_version_1,
                                 WAVM::I32,
                                 WAVM::I64,
                                 WAVM::I64,
                                 WAVM::I32,
                                 WAVM::I32,
                                 WAVM::I32) {
    logger->warn("Unimplemented ext_sandbox_invoke_version_1 was called");
    return {};
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_sandbox_memory_get_version_1",
                                 WAVM::I32,
                                 ext_sandbox_memory_get_version_1,
                                 WAVM::I32,
                                 WAVM::I32,
                                 WAVM::I32,
                                 WAVM::I32) {
    logger->warn("Unimplemented ext_sandbox_memory_get_version_1 was called");
    return {};
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_sandbox_memory_new_version_1",
                                 WAVM::I32,
                                 ext_sandbox_memory_new_version_1,
                                 WAVM::I32,
                                 WAVM::I32) {
    logger->warn("Unimplemented ext_sandbox_memory_new_version_1 was called");
    return {};
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_sandbox_memory_set_version_1",
                                 WAVM::I32,
                                 ext_sandbox_memory_set_version_1,
                                 WAVM::I32,
                                 WAVM::I32,
                                 WAVM::I32,
                                 WAVM::I32) {
    logger->warn("Unimplemented ext_sandbox_memory_set_version_1 was called");
    return {};
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(env,
                                 "ext_sandbox_memory_teardown_version_1",
                                 void,
                                 ext_sandbox_memory_teardown_version_1,
                                 WAVM::I32) {
    logger->warn(
        "Unimplemented ext_sandbox_memory_teardown_version_1 was called");
  }

}  // namespace kagome::runtime::wavm
