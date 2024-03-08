/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

// clang-format off
#define REGISTER_HOST_METHODS \
    REGISTER_HOST_METHOD(void, ext_allocator_free_version_1, WasmPointer) \
    REGISTER_HOST_METHOD(void, ext_crypto_start_batch_verify_version_1) \
    REGISTER_HOST_METHOD(void, ext_default_child_storage_clear_version_1, WasmSpan, WasmSpan) \
    REGISTER_HOST_METHOD(void, ext_default_child_storage_clear_prefix_version_1, WasmSpan, WasmSpan) \
    REGISTER_HOST_METHOD(int64_t, ext_default_child_storage_clear_prefix_version_2, int64_t, int64_t, int64_t) \
    REGISTER_HOST_METHOD(void, ext_default_child_storage_set_version_1, int64_t, int64_t, int64_t) \
    REGISTER_HOST_METHOD(void, ext_default_child_storage_storage_kill_version_1, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_default_child_storage_storage_kill_version_3, int64_t, int64_t) \
    REGISTER_HOST_METHOD(void, ext_logging_log_version_1, int32_t, int64_t, int64_t) \
    REGISTER_HOST_METHOD(void, ext_misc_print_hex_version_1, int64_t) \
    REGISTER_HOST_METHOD(void, ext_misc_print_num_version_1, int64_t) \
    REGISTER_HOST_METHOD(void, ext_misc_print_utf8_version_1, int64_t) \
    REGISTER_HOST_METHOD(void, ext_storage_append_version_1, int64_t, int64_t) \
    REGISTER_HOST_METHOD(void, ext_storage_clear_prefix_version_1, int64_t) \
    REGISTER_HOST_METHOD(void, ext_storage_clear_version_1, int64_t) \
    REGISTER_HOST_METHOD(void, ext_storage_commit_transaction_version_1) \
    REGISTER_HOST_METHOD(void, ext_storage_rollback_transaction_version_1) \
    REGISTER_HOST_METHOD(void, ext_storage_set_version_1, int64_t, int64_t) \
    REGISTER_HOST_METHOD(void, ext_storage_start_transaction_version_1) \
    REGISTER_HOST_METHOD(int32_t, ext_allocator_malloc_version_1, int32_t) \
    REGISTER_HOST_METHOD(int32_t, ext_crypto_ed25519_generate_version_1, int32_t, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_crypto_ed25519_verify_version_1, int32_t, int64_t, int32_t) \
    REGISTER_HOST_METHOD(int32_t, ext_crypto_finish_batch_verify_version_1) \
    REGISTER_HOST_METHOD(int32_t, ext_crypto_sr25519_generate_version_1, int32_t, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_crypto_sr25519_verify_version_1, int32_t, int64_t, int32_t) \
    REGISTER_HOST_METHOD(int32_t, ext_crypto_sr25519_verify_version_2, int32_t, int64_t, int32_t) \
    REGISTER_HOST_METHOD(int64_t, ext_crypto_ecdsa_public_keys_version_1, int32_t) \
    REGISTER_HOST_METHOD(int64_t, ext_crypto_ecdsa_sign_version_1, int32_t, int32_t, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_crypto_ecdsa_sign_prehashed_version_1, int32_t, int32_t, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_crypto_ecdsa_generate_version_1, int32_t, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_crypto_ecdsa_verify_version_1, int32_t, int64_t, int32_t) \
    REGISTER_HOST_METHOD(int32_t, ext_crypto_ecdsa_verify_prehashed_version_1, int32_t, int32_t, int32_t) \
    REGISTER_HOST_METHOD(int32_t, ext_crypto_ecdsa_verify_version_2, int32_t, int64_t, int32_t) \
    REGISTER_HOST_METHOD(int32_t, ext_default_child_storage_exists_version_1, int64_t, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_hashing_blake2_128_version_1, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_hashing_blake2_256_version_1, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_hashing_keccak_256_version_1, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_hashing_sha2_256_version_1, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_hashing_twox_64_version_1, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_hashing_twox_128_version_1, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_hashing_twox_256_version_1, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_logging_max_level_version_1) \
    REGISTER_HOST_METHOD(int32_t, ext_storage_exists_version_1, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_trie_blake2_256_ordered_root_version_1, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_trie_blake2_256_ordered_root_version_2, int64_t, int32_t) \
    REGISTER_HOST_METHOD(int32_t, ext_trie_keccak_256_ordered_root_version_2, int64_t, int32_t) \
    REGISTER_HOST_METHOD(int32_t, ext_trie_blake2_256_root_version_1, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_crypto_ed25519_public_keys_version_1, int32_t) \
    REGISTER_HOST_METHOD(int64_t, ext_crypto_ed25519_sign_version_1, int32_t, int32_t, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_crypto_secp256k1_ecdsa_recover_compressed_version_1, int32_t, int32_t) \
    REGISTER_HOST_METHOD(int64_t, ext_crypto_secp256k1_ecdsa_recover_compressed_version_2, int32_t, int32_t) \
    REGISTER_HOST_METHOD(int64_t, ext_crypto_secp256k1_ecdsa_recover_version_1, int32_t, int32_t) \
    REGISTER_HOST_METHOD(int64_t, ext_crypto_secp256k1_ecdsa_recover_version_2, int32_t, int32_t) \
    REGISTER_HOST_METHOD(int64_t, ext_crypto_sr25519_public_keys_version_1, int32_t) \
    REGISTER_HOST_METHOD(int64_t, ext_crypto_sr25519_sign_version_1, int32_t, int32_t, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_default_child_storage_get_version_1, int64_t, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_default_child_storage_next_key_version_1, int64_t, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_default_child_storage_read_version_1, int64_t, int64_t, int64_t, int32_t) \
    REGISTER_HOST_METHOD(int64_t, ext_default_child_storage_root_version_1, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_default_child_storage_root_version_2, int64_t, int32_t) \
    REGISTER_HOST_METHOD(int64_t, ext_misc_runtime_version_version_1, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_storage_changes_root_version_1, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_storage_clear_prefix_version_2, int64_t, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_storage_get_version_1,  int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_storage_next_key_version_1, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_storage_read_version_1, int64_t, int64_t, int32_t) \
    REGISTER_HOST_METHOD(int64_t, ext_storage_root_version_1) \
    REGISTER_HOST_METHOD(int64_t, ext_storage_root_version_2, int32_t) \
    REGISTER_HOST_METHOD(int32_t, ext_offchain_is_validator_version_1) \
    REGISTER_HOST_METHOD(int64_t, ext_offchain_submit_transaction_version_1, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_offchain_network_state_version_1) \
    REGISTER_HOST_METHOD(int64_t, ext_offchain_timestamp_version_1) \
    REGISTER_HOST_METHOD(void   , ext_offchain_sleep_until_version_1, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_offchain_random_seed_version_1) \
    REGISTER_HOST_METHOD(void   , ext_offchain_local_storage_set_version_1, int32_t, int64_t, int64_t) \
    REGISTER_HOST_METHOD(void   , ext_offchain_local_storage_clear_version_1, int32_t, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_offchain_local_storage_compare_and_set_version_1, int32_t, int64_t, int64_t, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_offchain_local_storage_get_version_1, int32_t, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_offchain_http_request_start_version_1, int64_t, int64_t, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_offchain_http_request_add_header_version_1, int32_t, int64_t, int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_offchain_http_request_write_body_version_1, int32_t,int64_t,int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_offchain_http_response_wait_version_1,int64_t,int64_t) \
    REGISTER_HOST_METHOD(int64_t, ext_offchain_http_response_headers_version_1, int32_t) \
    REGISTER_HOST_METHOD(int64_t, ext_offchain_http_response_read_body_version_1, int32_t, int64_t, int64_t) \
    REGISTER_HOST_METHOD(void   , ext_offchain_set_authorized_nodes_version_1, int64_t, int32_t) \
    REGISTER_HOST_METHOD(void   , ext_offchain_index_set_version_1, int64_t, int64_t) \
    REGISTER_HOST_METHOD(void   , ext_offchain_index_clear_version_1, int64_t) \
    REGISTER_HOST_METHOD(void   , ext_panic_handler_abort_on_panic_version_1, int64_t) \
    REGISTER_HOST_METHOD(int32_t, ext_crypto_bandersnatch_generate_version_1, int32_t, int64_t)
// clang-format on
