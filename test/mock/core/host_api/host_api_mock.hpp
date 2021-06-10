/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_RUNTIME_MOCK_HOST_API_HPP_
#define KAGOME_TEST_CORE_RUNTIME_MOCK_HOST_API_HPP_

#include "host_api/host_api.hpp"

#include <gmock/gmock.h>

namespace kagome::host_api {

  class HostApiMock final : public HostApi {
   public:
    MOCK_CONST_METHOD0(memory, std::shared_ptr<runtime::Memory>());
    MOCK_METHOD0(reset, void());

    MOCK_METHOD3(ext_storage_read_version_1,
                 runtime::WasmSpan(runtime::WasmSpan key,
                                   runtime::WasmSpan value_out,
                                   runtime::WasmOffset offset));
    MOCK_METHOD3(ext_logging_log_version_1,
                 void(runtime::WasmEnum level,
                      runtime::WasmSpan target,
                      runtime::WasmSpan message));

    MOCK_METHOD0(ext_logging_max_level_version_1, runtime::WasmEnum());

    MOCK_METHOD3(ext_crypto_sr25519_verify_version_1,
                 int32_t(runtime::WasmPointer sig_data,
                         runtime::WasmSpan msg,
                         runtime::WasmPointer pubkey_data));

    MOCK_METHOD3(ext_crypto_sr25519_verify_version_2,
                 int32_t(runtime::WasmPointer sig_data,
                         runtime::WasmSpan msg,
                         runtime::WasmPointer pubkey_data));

    MOCK_CONST_METHOD1(ext_misc_runtime_version_version_1,
                       runtime::WasmSpan(runtime::WasmSpan));

    MOCK_CONST_METHOD1(ext_misc_print_hex_version_1, void(runtime::WasmSpan));
    MOCK_CONST_METHOD1(ext_misc_print_num_version_1, void(uint64_t));
    MOCK_CONST_METHOD1(ext_misc_print_utf8_version_1, void(runtime::WasmSpan));

    // ------------------------ Storage extensions v1 ------------------------

    MOCK_METHOD2(ext_storage_set_version_1,
                 void(runtime::WasmSpan, runtime::WasmSpan));

    MOCK_METHOD1(ext_storage_get_version_1,
                 runtime::WasmSpan(runtime::WasmSpan));

    MOCK_METHOD1(ext_storage_clear_version_1, void(runtime::WasmSpan));

    MOCK_CONST_METHOD1(ext_storage_exists_version_1,
                       runtime::WasmSize(runtime::WasmSpan));

    MOCK_METHOD1(ext_storage_clear_prefix_version_1, void(runtime::WasmSpan));

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

    MOCK_METHOD1(ext_crypto_ed25519_public_keys_version_1,
                 runtime::WasmSpan(runtime::WasmSize key_type));

    MOCK_METHOD2(ext_crypto_ed25519_generate_version_1,
                 runtime::WasmPointer(runtime::WasmSize key_type,
                                      runtime::WasmSpan seed));

    MOCK_METHOD3(ext_crypto_ed25519_sign_version_1,
                 runtime::WasmSpan(runtime::WasmSize key_type,
                                   runtime::WasmPointer key,
                                   runtime::WasmSpan msg_data));

    MOCK_METHOD3(ext_crypto_ed25519_verify_version_1,
                 runtime::WasmSize(runtime::WasmPointer sig_data,
                                   runtime::WasmSpan msg,
                                   runtime::WasmPointer pubkey_data));

    MOCK_METHOD1(ext_crypto_sr25519_public_keys_version_1,
                 runtime::WasmSpan(runtime::WasmSize key_type));

    MOCK_METHOD2(ext_crypto_sr25519_generate_version_1,
                 runtime::WasmPointer(runtime::WasmSize key_type,
                                      runtime::WasmSpan seed));

    MOCK_METHOD3(ext_crypto_sr25519_sign_version_1,
                 runtime::WasmSpan(runtime::WasmSize key_type,
                                   runtime::WasmPointer key,
                                   runtime::WasmSpan msg_data));

    MOCK_METHOD0(ext_crypto_start_batch_verify_version_1, void());
    MOCK_METHOD0(ext_crypto_finish_batch_verify_version_1, int32_t());

    MOCK_METHOD2(ext_crypto_secp256k1_ecdsa_recover_version_1,
                 runtime::WasmSpan(runtime::WasmPointer sig,
                                   runtime::WasmPointer msg));

    MOCK_METHOD2(ext_crypto_secp256k1_ecdsa_recover_compressed_version_1,
                 runtime::WasmSpan(runtime::WasmPointer sig,
                                   runtime::WasmPointer msg));

    // ---------------------------- memory api v1 ----------------------------

    MOCK_METHOD1(ext_allocator_malloc_version_1,
                 runtime::WasmPointer(runtime::WasmSize));

    MOCK_METHOD1(ext_allocator_free_version_1, void(runtime::WasmPointer));
  };

}  // namespace kagome::host_api

#endif  // KAGOME_TEST_CORE_RUNTIME_MOCK_EXTENSION_HPP_
