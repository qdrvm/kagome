/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_RUNTIME_MOCK_HOST_API_HPP_
#define KAGOME_TEST_CORE_RUNTIME_MOCK_HOST_API_HPP_

#include "host_api/host_api.hpp"

#include <gmock/gmock.h>

namespace kagome::host_api {

  class HostApiMock : public HostApi {
   public:
    MOCK_CONST_METHOD0(memory, std::shared_ptr<runtime::WasmMemory>());
    MOCK_METHOD0(reset, void());
    MOCK_METHOD2(ext_clear_prefix,
                 void(runtime::WasmPointer prefix_data,
                      runtime::WasmSize prefix_length));
    MOCK_METHOD2(ext_clear_storage,
                 void(runtime::WasmPointer key_data,
                      runtime::WasmSize key_length));
    MOCK_CONST_METHOD2(ext_exists_storage,
                       runtime::WasmSize(runtime::WasmPointer key_data,
                                         runtime::WasmSize key_length));
    MOCK_METHOD3(ext_get_allocated_storage,
                 runtime::WasmPointer(runtime::WasmPointer key_data,
                                      runtime::WasmSize key_length,
                                      runtime::WasmPointer len_ptr));
    MOCK_METHOD5(ext_get_storage_into,
                 runtime::WasmSize(runtime::WasmPointer key_data,
                                   runtime::WasmSize key_length,
                                   runtime::WasmPointer value_data,
                                   runtime::WasmSize value_length,
                                   runtime::WasmSize value_offset));
    MOCK_METHOD3(ext_storage_read_version_1,
                 runtime::WasmSpan(runtime::WasmSpan key,
                                   runtime::WasmSpan value_out,
                                   runtime::WasmOffset offset));
    MOCK_METHOD4(ext_set_storage,
                 void(runtime::WasmPointer key_data,
                      runtime::WasmSize key_length,
                      runtime::WasmPointer value_data,
                      runtime::WasmSize value_length));
    MOCK_METHOD4(ext_blake2_256_enumerated_trie_root,
                 void(runtime::WasmPointer values_data,
                      runtime::WasmPointer lens_data,
                      runtime::WasmSize lens_length,
                      runtime::WasmPointer result));
    MOCK_METHOD2(ext_storage_changes_root,
                 runtime::WasmSize(runtime::WasmPointer parent_hash_data,
                                   runtime::WasmPointer result));
    MOCK_CONST_METHOD1(ext_storage_root, void(runtime::WasmPointer result));
    MOCK_METHOD1(ext_malloc, runtime::WasmPointer(runtime::WasmSize size));
    MOCK_METHOD1(ext_free, void(runtime::WasmPointer ptr));
    MOCK_METHOD2(ext_print_hex,
                 void(runtime::WasmPointer data, runtime::WasmSize length));
    MOCK_METHOD3(ext_logging_log_version_1,
                 void(runtime::WasmEnum level,
                      runtime::WasmSpan target,
                      runtime::WasmSpan message));
    MOCK_METHOD0(ext_logging_max_level_version_1, runtime::WasmEnum());
    MOCK_METHOD1(ext_print_num, void(uint64_t value));
    MOCK_METHOD2(ext_print_utf8,
                 void(runtime::WasmPointer utf8_data,
                      runtime::WasmSize utf8_length));
    MOCK_METHOD3(ext_blake2_128,
                 void(runtime::WasmPointer data,
                      runtime::WasmSize len,
                      runtime::WasmPointer out));
    MOCK_METHOD3(ext_blake2_256,
                 void(runtime::WasmPointer data,
                      runtime::WasmSize len,
                      runtime::WasmPointer out));
    MOCK_METHOD3(ext_keccak_256,
                 void(runtime::WasmPointer data,
                      runtime::WasmSize len,
                      runtime::WasmPointer out));
    MOCK_METHOD2(ext_ed25519_public_keys,
                 runtime::WasmSize(runtime::WasmSize key_type,
                                   runtime::WasmPointer out_ptr));

    MOCK_METHOD4(ext_ed25519_generate,
                 runtime::WasmSize(runtime::WasmSize key_type,
                                   runtime::WasmPointer seed_data,
                                   runtime::WasmSize seed_len,
                                   runtime::WasmPointer out_ptr));

    MOCK_METHOD5(ext_ed25519_sign,
                 runtime::WasmSize(runtime::WasmSize key_type,
                                   runtime::WasmPointer key,
                                   runtime::WasmPointer msg_data,
                                   runtime::WasmSize msg_len,
                                   runtime::WasmPointer out_ptr));

    MOCK_METHOD0(ext_start_batch_verify, void());

    MOCK_METHOD0(ext_finish_batch_verify, runtime::WasmSize());

    MOCK_METHOD4(ext_ed25519_verify,
                 runtime::WasmSize(runtime::WasmPointer msg_data,
                                   runtime::WasmSize msg_len,
                                   runtime::WasmPointer sig_data,
                                   runtime::WasmPointer pubkey_data));

    MOCK_METHOD2(ext_sr25519_public_keys,
                 runtime::WasmSize(runtime::WasmSize key_type,
                                   runtime::WasmPointer out_keys));

    MOCK_METHOD4(ext_sr25519_generate,
                 runtime::WasmSize(runtime::WasmSize key_type,
                                   runtime::WasmPointer seed_data,
                                   runtime::WasmSize seed_len,
                                   runtime::WasmPointer out_ptr));

    MOCK_METHOD5(ext_sr25519_sign,
                 runtime::WasmSize(runtime::WasmSize key_type,
                                   runtime::WasmPointer key,
                                   runtime::WasmPointer msg_data,
                                   runtime::WasmSize msg_len,
                                   runtime::WasmPointer out_ptr));

    MOCK_METHOD4(ext_sr25519_verify,
                 runtime::WasmSize(runtime::WasmPointer msg_data,
                                   runtime::WasmSize msg_len,
                                   runtime::WasmPointer sig_data,
                                   runtime::WasmPointer pubkey_data));
    MOCK_METHOD3(ext_twox_64,
                 void(runtime::WasmPointer data,
                      runtime::WasmSize len,
                      runtime::WasmPointer out));
    MOCK_METHOD3(ext_twox_128,
                 void(runtime::WasmPointer data,
                      runtime::WasmSize len,
                      runtime::WasmPointer out));
    MOCK_METHOD3(ext_twox_256,
                 void(runtime::WasmPointer data,
                      runtime::WasmSize len,
                      runtime::WasmPointer out));
    MOCK_CONST_METHOD0(ext_chain_id, uint64_t());
    MOCK_CONST_METHOD1(ext_misc_runtime_version_version_1,
                       runtime::WasmResult(runtime::WasmSpan));

    MOCK_CONST_METHOD1(ext_misc_print_hex_version_1, void(runtime::WasmSpan));
    MOCK_CONST_METHOD1(ext_misc_print_num_version_1, void(uint64_t));
    MOCK_CONST_METHOD1(ext_misc_print_utf8_version_1, void(runtime::WasmSpan));

    MOCK_METHOD0(ext_storage_start_transaction, void());
    MOCK_METHOD0(ext_storage_rollback_transaction, void());
    MOCK_METHOD0(ext_storage_commit_transaction, void());

    // ------------------------ Storage extensions v1 ------------------------

    MOCK_METHOD2(ext_storage_set_version_1,
                 void(runtime::WasmSpan, runtime::WasmSpan));

    MOCK_METHOD1(ext_storage_get_version_1,
                 runtime::WasmSpan(runtime::WasmSpan));

    MOCK_METHOD1(ext_storage_clear_version_1, void(runtime::WasmSpan));

    MOCK_CONST_METHOD1(ext_storage_exists_version_1,
                       runtime::WasmSize(runtime::WasmSpan));

    MOCK_METHOD1(ext_storage_clear_prefix_version_1, void(runtime::WasmSpan));
    MOCK_METHOD2(ext_storage_clear_prefix_version_2,
                 runtime::WasmSpan(runtime::WasmSpan, runtime::WasmSpan));

    MOCK_METHOD0(ext_storage_root_version_1, runtime::WasmSpan());

    MOCK_METHOD1(ext_storage_changes_root_version_1,
                 runtime::WasmSpan(runtime::WasmSpan));

    MOCK_CONST_METHOD1(ext_storage_next_key_version_1,
                       runtime::WasmSpan(runtime::WasmSpan));

    MOCK_CONST_METHOD2(ext_storage_append_version_1,
                       void(runtime::WasmSpan, runtime::WasmSpan));

    MOCK_METHOD1(ext_trie_blake2_256_root_version_1,
                 runtime::WasmPointer(runtime::WasmSpan values_data));

    MOCK_METHOD1(ext_trie_blake2_256_ordered_root_version_1,
                 runtime::WasmPointer(runtime::WasmSpan));

    // -------------------- hashing methods v1 --------------------

    MOCK_METHOD1(ext_hashing_keccak_256_version_1,
                 runtime::WasmPointer(runtime::WasmSpan));

    MOCK_METHOD1(ext_hashing_sha2_256_version_1,
                 runtime::WasmPointer(runtime::WasmSpan));

    MOCK_METHOD1(ext_hashing_blake2_128_version_1,
                 runtime::WasmPointer(runtime::WasmSpan));

    MOCK_METHOD1(ext_hashing_blake2_256_version_1,
                 runtime::WasmPointer(runtime::WasmSpan));

    MOCK_METHOD1(ext_hashing_twox_64_version_1,
                 runtime::WasmPointer(runtime::WasmSpan));

    MOCK_METHOD1(ext_hashing_twox_128_version_1,
                 runtime::WasmPointer(runtime::WasmSpan));

    MOCK_METHOD1(ext_hashing_twox_256_version_1,
                 runtime::WasmPointer(runtime::WasmSpan));

    // -------------------------Crypto extensions v1---------------------

    MOCK_METHOD1(ext_ed25519_public_keys_v1,
                 runtime::WasmSpan(runtime::WasmSize key_type));

    MOCK_METHOD2(ext_ed25519_generate_v1,
                 runtime::WasmPointer(runtime::WasmSize key_type,
                                      runtime::WasmSpan seed));

    MOCK_METHOD3(ext_ed25519_sign_v1,
                 runtime::WasmSpan(runtime::WasmSize key_type,
                                   runtime::WasmPointer key,
                                   runtime::WasmSpan msg_data));

    MOCK_METHOD3(ext_ed25519_verify_v1,
                 runtime::WasmSize(runtime::WasmPointer sig_data,
                                   runtime::WasmSpan msg,
                                   runtime::WasmPointer pubkey_data));

    MOCK_METHOD1(ext_sr25519_public_keys_v1,
                 runtime::WasmSpan(runtime::WasmSize key_type));

    MOCK_METHOD2(ext_sr25519_generate_v1,
                 runtime::WasmPointer(runtime::WasmSize key_type,
                                      runtime::WasmSpan seed));

    MOCK_METHOD3(ext_sr25519_sign_v1,
                 runtime::WasmSpan(runtime::WasmSize key_type,
                                   runtime::WasmPointer key,
                                   runtime::WasmSpan msg_data));

    MOCK_METHOD3(ext_sr25519_verify_v1,
                 runtime::WasmSize(runtime::WasmPointer sig_data,
                                   runtime::WasmSpan msg,
                                   runtime::WasmPointer pubkey_data));

    MOCK_METHOD2(ext_crypto_secp256k1_ecdsa_recover_v1,
                 runtime::WasmSpan(runtime::WasmPointer sig,
                                   runtime::WasmPointer msg));

    MOCK_METHOD2(ext_crypto_secp256k1_ecdsa_recover_compressed_v1,
                 runtime::WasmSpan(runtime::WasmPointer sig,
                                   runtime::WasmPointer msg));

    // ---------------------------- memory api v1 ----------------------------

    MOCK_METHOD1(ext_allocator_malloc_version_1,
                 runtime::WasmPointer(runtime::WasmSize));

    MOCK_METHOD1(ext_allocator_free_version_1, void(runtime::WasmPointer));
  };

}  // namespace kagome::host_api

#endif  // KAGOME_TEST_CORE_RUNTIME_MOCK_EXTENSION_HPP_
