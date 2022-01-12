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

#undef WAVM_DEFINE_INTRINSIC_FUNCTION
// ContextRuntimeData is required by WAVM
#define WAVM_DEFINE_INTRINSIC_FUNCTION(Result, cName, ...)            \
  Result cName(WAVM::Runtime::ContextRuntimeData *contextRuntimeData, \
               ##__VA_ARGS__)

#define WAVM_DEFINE_INTRINSIC_FUNCTION_STUB(Result, cName, ...)            \
  Result cName(WAVM::Runtime::ContextRuntimeData *contextRuntimeData,      \
               ##__VA_ARGS__) {                                            \
    logger->warn("Unimplemented Host API function " #cName " was called"); \
    return Result();                                                       \
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_allocator_free_version_1,
                                 WAVM::I32 address) {
    return peekHostApi()->ext_allocator_free_version_1(address);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_allocator_malloc_version_1,
                                 WAVM::I32 size) {
    return peekHostApi()->ext_allocator_malloc_version_1(size);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_crypto_ed25519_generate_version_1,
                                 WAVM::I32 keytype,
                                 WAVM::I64 seed) {
    return peekHostApi()->ext_crypto_ed25519_generate_version_1(keytype, seed);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_crypto_ed25519_public_keys_version_1,
                                 WAVM::I32 key_type) {
    return peekHostApi()->ext_crypto_ed25519_public_keys_version_1(key_type);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_crypto_ed25519_sign_version_1,
                                 WAVM::I32 key_type,
                                 WAVM::I32 key,
                                 WAVM::I64 msg_data) {
    return peekHostApi()->ext_crypto_ed25519_sign_version_1(
        key_type, key, msg_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_crypto_ed25519_verify_version_1,
                                 WAVM::I32 sig_data,
                                 WAVM::I64 msg,
                                 WAVM::I32 pubkey_data) {
    return peekHostApi()->ext_crypto_ed25519_verify_version_1(
        sig_data, msg, pubkey_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_crypto_finish_batch_verify_version_1) {
    return peekHostApi()->ext_crypto_finish_batch_verify_version_1();
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_crypto_secp256k1_ecdsa_recover_version_1,
                                 WAVM::I32 sig,
                                 WAVM::I32 msg) {
    return peekHostApi()->ext_crypto_secp256k1_ecdsa_recover_version_1(sig,
                                                                       msg);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_crypto_secp256k1_ecdsa_recover_version_2,
                                 WAVM::I32 sig,
                                 WAVM::I32 msg) {
    return peekHostApi()->ext_crypto_secp256k1_ecdsa_recover_version_2(sig,
                                                                       msg);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(
      WAVM::I64,
      ext_crypto_secp256k1_ecdsa_recover_compressed_version_1,
      WAVM::I32 sig,
      WAVM::I32 msg) {
    return peekHostApi()
        ->ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(sig, msg);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(
      WAVM::I64,
      ext_crypto_secp256k1_ecdsa_recover_compressed_version_2,
      WAVM::I32 sig,
      WAVM::I32 msg) {
    return peekHostApi()
        ->ext_crypto_secp256k1_ecdsa_recover_compressed_version_2(sig, msg);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_crypto_sr25519_generate_version_1,
                                 WAVM::I32 key_type,
                                 WAVM::I64 seed) {
    return peekHostApi()->ext_crypto_sr25519_generate_version_1(key_type, seed);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_crypto_sr25519_public_keys_version_1,
                                 WAVM::I32 key_type) {
    return peekHostApi()->ext_crypto_sr25519_public_keys_version_1(key_type);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_crypto_sr25519_sign_version_1,
                                 WAVM::I32 key_type,
                                 WAVM::I32 key,
                                 WAVM::I64 msg_data) {
    return peekHostApi()->ext_crypto_sr25519_sign_version_1(
        key_type, key, msg_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_crypto_sr25519_verify_version_1,
                                 WAVM::I32 sig_data,
                                 WAVM::I64 msg,
                                 WAVM::I32 pubkey_data) {
    return peekHostApi()->ext_crypto_sr25519_verify_version_1(
        sig_data, msg, pubkey_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_crypto_sr25519_verify_version_2,
                                 WAVM::I32 sig_data,
                                 WAVM::I64 msg,
                                 WAVM::I32 pubkey_data) {
    return peekHostApi()->ext_crypto_sr25519_verify_version_2(
        sig_data, msg, pubkey_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_crypto_ecdsa_generate_version_1,
                                 WAVM::I32 key_type,
                                 WAVM::I64 seed) {
    return peekHostApi()->ext_crypto_ecdsa_generate_version_1(key_type, seed);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_crypto_ecdsa_public_keys_version_1,
                                 WAVM::I32 key_type) {
    return peekHostApi()->ext_crypto_ecdsa_public_keys_version_1(key_type);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_crypto_ecdsa_sign_version_1,
                                 WAVM::I32 key_type,
                                 WAVM::I32 key,
                                 WAVM::I64 msg_data) {
    return peekHostApi()->ext_crypto_ecdsa_sign_version_1(
        key_type, key, msg_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_crypto_ecdsa_verify_version_1,
                                 WAVM::I32 sig_data,
                                 WAVM::I64 msg,
                                 WAVM::I32 pubkey_data) {
    return peekHostApi()->ext_crypto_ecdsa_verify_version_1(
        sig_data, msg, pubkey_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_crypto_start_batch_verify_version_1) {
    return peekHostApi()->ext_crypto_start_batch_verify_version_1();
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_trie_blake2_256_ordered_root_version_1,
                                 WAVM::I64 values_data) {
    return peekHostApi()->ext_trie_blake2_256_ordered_root_version_1(
        values_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_misc_print_hex_version_1,
                                 WAVM::I64 values_data) {
    return peekHostApi()->ext_misc_print_hex_version_1(values_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_misc_print_num_version_1,
                                 WAVM::I64 values_data) {
    return peekHostApi()->ext_misc_print_num_version_1(values_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_misc_print_utf8_version_1,
                                 WAVM::I64 values_data) {
    return peekHostApi()->ext_misc_print_utf8_version_1(values_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_misc_runtime_version_version_1,
                                 WAVM::I64 values_data) {
    return peekHostApi()->ext_misc_runtime_version_version_1(values_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_default_child_storage_clear_version_1,
                                 WAVM::I64 child_storage_key,
                                 WAVM::I64 key) {
    return peekHostApi()->ext_default_child_storage_clear_version_1(
        child_storage_key, key);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_default_child_storage_read_version_1,
                                 WAVM::I64 child_storage_key,
                                 WAVM::I64 key,
                                 WAVM::I64 value_out,
                                 WAVM::I32 offset) {
    return peekHostApi()->ext_default_child_storage_read_version_1(
        child_storage_key, key, value_out, offset);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_default_child_storage_exists_version_1,
                                 WAVM::I64 child_storage_key,
                                 WAVM::I64 key) {
    return peekHostApi()->ext_default_child_storage_exists_version_1(
        child_storage_key, key);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_default_child_storage_get_version_1,
                                 WAVM::I64 child_storage_key,
                                 WAVM::I64 key) {
    return peekHostApi()->ext_default_child_storage_get_version_1(
        child_storage_key, key);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_default_child_storage_next_key_version_1,
                                 WAVM::I64 child_storage_key,
                                 WAVM::I64 key) {
    return peekHostApi()->ext_default_child_storage_next_key_version_1(
        child_storage_key, key);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(
      void,
      ext_default_child_storage_clear_prefix_version_1,
      WAVM::I64 child_storage_key,
      WAVM::I64 prefix) {
    return peekHostApi()->ext_default_child_storage_clear_prefix_version_1(
        child_storage_key, prefix);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_default_child_storage_root_version_1,
                                 WAVM::I64 child_storage_key) {
    return peekHostApi()->ext_default_child_storage_root_version_1(
        child_storage_key);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_default_child_storage_set_version_1,
                                 WAVM::I64 child_storage_key,
                                 WAVM::I64 key,
                                 WAVM::I64 value) {
    return peekHostApi()->ext_default_child_storage_set_version_1(
        child_storage_key, key, value);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(
      void,
      ext_default_child_storage_storage_kill_version_1,
      WAVM::I64 child_storage_key) {
    return peekHostApi()->ext_default_child_storage_storage_kill_version_1(
        child_storage_key);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_hashing_blake2_128_version_1,
                                 WAVM::I64 data) {
    return peekHostApi()->ext_hashing_blake2_128_version_1(data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_hashing_blake2_256_version_1,
                                 WAVM::I64 data) {
    return peekHostApi()->ext_hashing_blake2_256_version_1(data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_hashing_keccak_256_version_1,
                                 WAVM::I64 data) {
    return peekHostApi()->ext_hashing_keccak_256_version_1(data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_hashing_sha2_256_version_1,
                                 WAVM::I64 data) {
    return peekHostApi()->ext_hashing_sha2_256_version_1(data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_hashing_twox_128_version_1,
                                 WAVM::I64 data) {
    return peekHostApi()->ext_hashing_twox_128_version_1(data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_hashing_twox_64_version_1,
                                 WAVM::I64 data) {
    return peekHostApi()->ext_hashing_twox_64_version_1(data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_hashing_twox_256_version_1,
                                 WAVM::I64 data) {
    return peekHostApi()->ext_hashing_twox_256_version_1(data);
  }

  // --------------------------- Offchain extension ----------------------------

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_offchain_is_validator_version_1) {
    return peekHostApi()->ext_offchain_is_validator_version_1();
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_offchain_submit_transaction_version_1,
                                 WAVM::I64 xt) {
    return peekHostApi()->ext_offchain_submit_transaction_version_1(xt);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_offchain_network_state_version_1) {
    return peekHostApi()->ext_offchain_network_state_version_1();
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64, ext_offchain_timestamp_version_1) {
    return peekHostApi()->ext_offchain_timestamp_version_1();
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_offchain_sleep_until_version_1,
                                 WAVM::I64 deadline) {
    return peekHostApi()->ext_offchain_sleep_until_version_1(deadline);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_offchain_random_seed_version_1) {
    return peekHostApi()->ext_offchain_random_seed_version_1();
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_offchain_local_storage_set_version_1,
                                 WAVM::I32 kind,
                                 WAVM::I64 key,
                                 WAVM::I64 value) {
    return peekHostApi()->ext_offchain_local_storage_set_version_1(
        kind, key, value);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_offchain_local_storage_clear_version_1,
                                 WAVM::I32 kind,
                                 WAVM::I64 key) {
    return peekHostApi()->ext_offchain_local_storage_clear_version_1(kind, key);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(
      WAVM::I32,
      ext_offchain_local_storage_compare_and_set_version_1,
      WAVM::I32 kind,
      WAVM::I64 key,
      WAVM::I64 expected,
      WAVM::I64 value) {
    return peekHostApi()->ext_offchain_local_storage_compare_and_set_version_1(
        kind, key, expected, value);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_offchain_local_storage_get_version_1,
                                 WAVM::I32 kind,
                                 WAVM::I64 key) {
    return peekHostApi()->ext_offchain_local_storage_get_version_1(kind, key);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_offchain_http_request_start_version_1,
                                 WAVM::I64 method,
                                 WAVM::I64 uri,
                                 WAVM::I64 meta) {
    return peekHostApi()->ext_offchain_http_request_start_version_1(
        method, uri, meta);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_offchain_http_request_add_header_version_1,
                                 WAVM::I32 request_id,
                                 WAVM::I64 name,
                                 WAVM::I64 value) {
    return peekHostApi()->ext_offchain_http_request_add_header_version_1(
        request_id, name, value);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_offchain_http_request_write_body_version_1,
                                 WAVM::I32 request_id,
                                 WAVM::I64 chunk,
                                 WAVM::I64 deadline) {
    return peekHostApi()->ext_offchain_http_request_write_body_version_1(
        request_id, chunk, deadline);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_offchain_http_response_wait_version_1,
                                 WAVM::I64 ids,
                                 WAVM::I64 deadline) {
    return peekHostApi()->ext_offchain_http_response_wait_version_1(ids,
                                                                    deadline);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_offchain_http_response_headers_version_1,
                                 WAVM::I32 request_id) {
    return peekHostApi()->ext_offchain_http_response_headers_version_1(
        request_id);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_offchain_http_response_read_body_version_1,
                                 WAVM::I32 request_id,
                                 WAVM::I64 buffer,
                                 WAVM::I64 deadline) {
    return peekHostApi()->ext_offchain_http_response_read_body_version_1(
        request_id, buffer, deadline);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_offchain_set_authorized_nodes_version_1,
                                 WAVM::I64 nodes,
                                 WAVM::I32 authorized_only) {
    return peekHostApi()->ext_offchain_set_authorized_nodes_version_1(
        nodes, authorized_only);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_offchain_index_set_version_1,
                                 WAVM::I64 key,
                                 WAVM::I64 value) {
    return peekHostApi()->ext_offchain_index_set_version_1(key, value);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_offchain_index_clear_version_1,
                                 WAVM::I64 key) {
    return peekHostApi()->ext_offchain_index_clear_version_1(key);
  }

  // ---------------------------------------------------------------------------

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_storage_append_version_1,
                                 WAVM::I64 key,
                                 WAVM::I64 value) {
    return peekHostApi()->ext_storage_append_version_1(key, value);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_storage_changes_root_version_1,
                                 WAVM::I64 parent_hash) {
    return peekHostApi()->ext_storage_changes_root_version_1(parent_hash);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_storage_clear_version_1,
                                 WAVM::I64 key_data) {
    return peekHostApi()->ext_storage_clear_version_1(key_data);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_storage_clear_prefix_version_1,
                                 WAVM::I64 prefix) {
    return peekHostApi()->ext_storage_clear_prefix_version_1(prefix);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_storage_clear_prefix_version_2,
                                 WAVM::I64 prefix,
                                 WAVM::I64 limit) {
    return peekHostApi()->ext_storage_clear_prefix_version_2(prefix, limit);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_storage_commit_transaction_version_1) {
    return peekHostApi()->ext_storage_commit_transaction_version_1();
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_storage_get_version_1,
                                 WAVM::I64 key) {
    return peekHostApi()->ext_storage_get_version_1(key);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_storage_next_key_version_1,
                                 WAVM::I64 key) {
    return peekHostApi()->ext_storage_next_key_version_1(key);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64,
                                 ext_storage_read_version_1,
                                 WAVM::I64 key,
                                 WAVM::I64 value_out,
                                 WAVM::I32 offset) {
    return peekHostApi()->ext_storage_read_version_1(key, value_out, offset);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_storage_rollback_transaction_version_1) {
    return peekHostApi()->ext_storage_rollback_transaction_version_1();
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I64, ext_storage_root_version_1) {
    return peekHostApi()->ext_storage_root_version_1();
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_storage_set_version_1,
                                 WAVM::I64 key,
                                 WAVM::I64 value) {
    return peekHostApi()->ext_storage_set_version_1(key, value);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_storage_start_transaction_version_1) {
    return peekHostApi()->ext_storage_start_transaction_version_1();
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_storage_exists_version_1,
                                 WAVM::I64 key) {
    return peekHostApi()->ext_storage_exists_version_1(key);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(void,
                                 ext_logging_log_version_1,
                                 WAVM::I32 level,
                                 WAVM::I64 target,
                                 WAVM::I64 message) {
    return peekHostApi()->ext_logging_log_version_1(level, target, message);
  }

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32, ext_logging_max_level_version_1) {
    return peekHostApi()->ext_logging_max_level_version_1();
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

  WAVM_DEFINE_INTRINSIC_FUNCTION(WAVM::I32,
                                 ext_trie_blake2_256_root_version_1,
                                 WAVM::I64 values_data) {
    return peekHostApi()->ext_trie_blake2_256_root_version_1(values_data);
  }

  void registerHostApiMethods(IntrinsicModule &module) {
    if (logger == nullptr)
      logger = log::createLogger("Host API wrappers", "wavm");

#define REGISTER_HOST_INTRINSIC(Ret, name, ...) \
  module.addFunction(                           \
      #name, &name, WAVM::IR::FunctionType{{Ret}, {__VA_ARGS__}});

    auto I32 = WAVM::IR::ValueType::i32;
    auto I64 = WAVM::IR::ValueType::i64;
    // clang-format off
    REGISTER_HOST_INTRINSIC(, ext_allocator_free_version_1, I32)
    REGISTER_HOST_INTRINSIC(, ext_crypto_start_batch_verify_version_1)
    REGISTER_HOST_INTRINSIC(, ext_default_child_storage_clear_version_1, I64, I64)
    REGISTER_HOST_INTRINSIC(, ext_default_child_storage_clear_prefix_version_1, I64, I64)
    REGISTER_HOST_INTRINSIC(, ext_default_child_storage_set_version_1, I64, I64, I64)
    REGISTER_HOST_INTRINSIC(, ext_default_child_storage_storage_kill_version_1, I64)
    REGISTER_HOST_INTRINSIC(, ext_logging_log_version_1, I32, I64, I64)
    REGISTER_HOST_INTRINSIC(, ext_misc_print_hex_version_1, I64)
    REGISTER_HOST_INTRINSIC(, ext_misc_print_num_version_1, I64)
    REGISTER_HOST_INTRINSIC(, ext_misc_print_utf8_version_1, I64)
    REGISTER_HOST_INTRINSIC(, ext_sandbox_instance_teardown_version_1, I32)
    REGISTER_HOST_INTRINSIC(, ext_sandbox_memory_teardown_version_1, I32)
    REGISTER_HOST_INTRINSIC(, ext_storage_append_version_1, I64, I64)
    REGISTER_HOST_INTRINSIC(, ext_storage_clear_prefix_version_1, I64)
    REGISTER_HOST_INTRINSIC(, ext_storage_clear_version_1, I64)
    REGISTER_HOST_INTRINSIC(, ext_storage_commit_transaction_version_1)
    REGISTER_HOST_INTRINSIC(, ext_storage_rollback_transaction_version_1)
    REGISTER_HOST_INTRINSIC(, ext_storage_set_version_1, I64, I64)
    REGISTER_HOST_INTRINSIC(, ext_storage_start_transaction_version_1)
    REGISTER_HOST_INTRINSIC(I32, ext_allocator_malloc_version_1, I32)
    REGISTER_HOST_INTRINSIC(I32, ext_crypto_ed25519_generate_version_1, I32, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_crypto_ed25519_verify_version_1, I32, I64, I32)
    REGISTER_HOST_INTRINSIC(I32, ext_crypto_finish_batch_verify_version_1)
    REGISTER_HOST_INTRINSIC(I32, ext_crypto_sr25519_generate_version_1, I32, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_crypto_sr25519_verify_version_1, I32, I64, I32)
    REGISTER_HOST_INTRINSIC(I32, ext_crypto_sr25519_verify_version_2, I32, I64, I32)
    REGISTER_HOST_INTRINSIC(I64, ext_crypto_ecdsa_public_keys_version_1, I32)
    REGISTER_HOST_INTRINSIC(I64, ext_crypto_ecdsa_sign_version_1, I32, I32, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_crypto_ecdsa_generate_version_1, I32, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_crypto_ecdsa_verify_version_1, I32, I64, I32)
    REGISTER_HOST_INTRINSIC(I32, ext_default_child_storage_exists_version_1, I64, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_hashing_blake2_128_version_1, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_hashing_blake2_256_version_1, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_hashing_keccak_256_version_1, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_hashing_sha2_256_version_1, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_hashing_twox_64_version_1, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_hashing_twox_128_version_1, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_hashing_twox_256_version_1, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_logging_max_level_version_1)
    REGISTER_HOST_INTRINSIC(I32, ext_sandbox_instantiate_version_1, I32, I64, I64, I32)
    REGISTER_HOST_INTRINSIC(I32, ext_sandbox_invoke_version_1, I32, I64, I64, I32, I32, I32)
    REGISTER_HOST_INTRINSIC(I32, ext_sandbox_memory_get_version_1, I32, I32, I32, I32)
    REGISTER_HOST_INTRINSIC(I32, ext_sandbox_memory_new_version_1, I32, I32)
    REGISTER_HOST_INTRINSIC(I32, ext_sandbox_memory_set_version_1, I32, I32, I32, I32)
    REGISTER_HOST_INTRINSIC(I32, ext_storage_exists_version_1, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_trie_blake2_256_ordered_root_version_1, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_trie_blake2_256_root_version_1, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_crypto_ed25519_public_keys_version_1, I32)
    REGISTER_HOST_INTRINSIC(I64, ext_crypto_ed25519_sign_version_1, I32, I32, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_crypto_secp256k1_ecdsa_recover_compressed_version_1, I32, I32)
    REGISTER_HOST_INTRINSIC(I64, ext_crypto_secp256k1_ecdsa_recover_compressed_version_2, I32, I32)
    REGISTER_HOST_INTRINSIC(I64, ext_crypto_secp256k1_ecdsa_recover_version_1, I32, I32)
    REGISTER_HOST_INTRINSIC(I64, ext_crypto_secp256k1_ecdsa_recover_version_2, I32, I32)
    REGISTER_HOST_INTRINSIC(I64, ext_crypto_sr25519_public_keys_version_1, I32)
    REGISTER_HOST_INTRINSIC(I64, ext_crypto_sr25519_sign_version_1, I32, I32, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_default_child_storage_get_version_1, I64, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_default_child_storage_next_key_version_1, I64, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_default_child_storage_read_version_1, I64, I64, I64, I32)
    REGISTER_HOST_INTRINSIC(I64, ext_default_child_storage_root_version_1, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_misc_runtime_version_version_1, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_storage_changes_root_version_1, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_storage_clear_prefix_version_2, I64, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_storage_get_version_1,  I64)
    REGISTER_HOST_INTRINSIC(I64, ext_storage_next_key_version_1, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_storage_read_version_1, I64, I64, I32)
    REGISTER_HOST_INTRINSIC(I64, ext_storage_root_version_1)

    // -------------------------- Offchain extension ---------------------------
    REGISTER_HOST_INTRINSIC(I32, ext_offchain_is_validator_version_1)
    REGISTER_HOST_INTRINSIC(I64, ext_offchain_submit_transaction_version_1, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_offchain_network_state_version_1)
    REGISTER_HOST_INTRINSIC(I64, ext_offchain_timestamp_version_1)
    REGISTER_HOST_INTRINSIC(   , ext_offchain_sleep_until_version_1, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_offchain_random_seed_version_1)
    REGISTER_HOST_INTRINSIC(   , ext_offchain_local_storage_set_version_1, I32, I64, I64)
    REGISTER_HOST_INTRINSIC(   , ext_offchain_local_storage_clear_version_1, I32, I64)
    REGISTER_HOST_INTRINSIC(I32, ext_offchain_local_storage_compare_and_set_version_1, I32, I64, I64, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_offchain_local_storage_get_version_1, I32, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_offchain_http_request_start_version_1, I64, I64, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_offchain_http_request_add_header_version_1, I32, I64, I64)
    REGISTER_HOST_INTRINSIC(I64, ext_offchain_http_request_write_body_version_1, I32,I64,I64)
    REGISTER_HOST_INTRINSIC(I64, ext_offchain_http_response_wait_version_1,I64,I64)
    REGISTER_HOST_INTRINSIC(I64, ext_offchain_http_response_headers_version_1, I32)
    REGISTER_HOST_INTRINSIC(I64, ext_offchain_http_response_read_body_version_1, I32, I64, I64)
    REGISTER_HOST_INTRINSIC(   , ext_offchain_set_authorized_nodes_version_1, I64, I32)
    REGISTER_HOST_INTRINSIC(   , ext_offchain_index_set_version_1, I64, I64)
    REGISTER_HOST_INTRINSIC(   , ext_offchain_index_clear_version_1, I64)

    // clang-format on
  }

}  // namespace kagome::runtime::wavm
