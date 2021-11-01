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
    MOCK_METHOD(std::shared_ptr<runtime::Memory>, memory, (), (const));

    MOCK_METHOD(void, reset, (), (override));

    MOCK_METHOD(runtime::WasmSpan,
                ext_storage_read_version_1,
                (runtime::WasmSpan key,
                 runtime::WasmSpan value_out,
                 runtime::WasmOffset offset),
                (override));

    MOCK_METHOD(void,
                ext_logging_log_version_1,
                (runtime::WasmEnum level,
                 runtime::WasmSpan target,
                 runtime::WasmSpan message),
                (override));

    MOCK_METHOD(runtime::WasmEnum,
                ext_logging_max_level_version_1,
                (),
                (override));

    MOCK_METHOD(int32_t,
                ext_crypto_sr25519_verify_version_1,
                (runtime::WasmPointer sig_data,
                 runtime::WasmSpan msg,
                 runtime::WasmPointer pubkey_data),
                (override));

    MOCK_METHOD(int32_t,
                ext_crypto_sr25519_verify_version_2,
                (runtime::WasmPointer sig_data,
                 runtime::WasmSpan msg,
                 runtime::WasmPointer pubkey_data),
                (override));

    MOCK_METHOD(runtime::WasmSpan,
                ext_misc_runtime_version_version_1,
                (runtime::WasmSpan),
                (const, override));

    MOCK_METHOD(void,
                ext_misc_print_hex_version_1,
                (runtime::WasmSpan),
                (const, override));

    MOCK_METHOD(void,
                ext_misc_print_num_version_1,
                (uint64_t),
                (const, override));

    MOCK_METHOD(void,
                ext_misc_print_utf8_version_1,
                (runtime::WasmSpan),
                (const, override));

    // ------------------------ Storage extensions v1 ------------------------

    MOCK_METHOD(void,
                ext_storage_set_version_1,
                (runtime::WasmSpan, runtime::WasmSpan),
                (override));

    MOCK_METHOD(runtime::WasmSpan,
                ext_storage_get_version_1,
                (runtime::WasmSpan),
                (override));

    MOCK_METHOD(void,
                ext_storage_clear_version_1,
                (runtime::WasmSpan),
                (override));

    MOCK_METHOD(runtime::WasmSize,
                ext_storage_exists_version_1,
                (runtime::WasmSpan),
                (const, override));

    MOCK_METHOD(void,
                ext_storage_clear_prefix_version_1,
                (runtime::WasmSpan),
                (override));

    MOCK_METHOD(runtime::WasmSpan,
                ext_storage_clear_prefix_version_2,
                (runtime::WasmSpan, runtime::WasmSpan),
                (override));

    MOCK_METHOD(runtime::WasmSpan, ext_storage_root_version_1, (), (override));

    MOCK_METHOD(runtime::WasmSpan,
                ext_storage_changes_root_version_1,
                (runtime::WasmSpan),
                (override));

    MOCK_METHOD(runtime::WasmSpan,
                ext_storage_next_key_version_1,
                (runtime::WasmSpan),
                (const, override));

    MOCK_METHOD(void,
                ext_storage_append_version_1,
                (runtime::WasmSpan, runtime::WasmSpan),
                (const, override));

    MOCK_METHOD(void, ext_storage_start_transaction_version_1, (), (override));

    MOCK_METHOD(void,
                ext_storage_rollback_transaction_version_1,
                (),
                (override));

    MOCK_METHOD(void, ext_storage_commit_transaction_version_1, (), (override));

    MOCK_METHOD(runtime::WasmPointer,
                ext_trie_blake2_256_root_version_1,
                (runtime::WasmSpan values_data),
                (override));

    MOCK_METHOD(runtime::WasmPointer,
                ext_trie_blake2_256_ordered_root_version_1,
                (runtime::WasmSpan),
                (override));

    // -------------------- hashing methods v1 --------------------

    MOCK_METHOD(runtime::WasmPointer,
                ext_hashing_keccak_256_version_1,
                (runtime::WasmSpan),
                (override));

    MOCK_METHOD(runtime::WasmPointer,
                ext_hashing_sha2_256_version_1,
                (runtime::WasmSpan),
                (override));

    MOCK_METHOD(runtime::WasmPointer,
                ext_hashing_blake2_128_version_1,
                (runtime::WasmSpan),
                (override));

    MOCK_METHOD(runtime::WasmPointer,
                ext_hashing_blake2_256_version_1,
                (runtime::WasmSpan),
                (override));

    MOCK_METHOD(runtime::WasmPointer,
                ext_hashing_twox_64_version_1,
                (runtime::WasmSpan),
                (override));

    MOCK_METHOD(runtime::WasmPointer,
                ext_hashing_twox_128_version_1,
                (runtime::WasmSpan),
                (override));

    MOCK_METHOD(runtime::WasmPointer,
                ext_hashing_twox_256_version_1,
                (runtime::WasmSpan),
                (override));

    // -------------------------Crypto extensions v1---------------------

    MOCK_METHOD(runtime::WasmSpan,
                ext_crypto_ed25519_public_keys_version_1,
                (runtime::WasmSize key_type),
                (override));

    MOCK_METHOD(runtime::WasmPointer,
                ext_crypto_ed25519_generate_version_1,
                (runtime::WasmSize key_type, runtime::WasmSpan seed),
                (override));

    MOCK_METHOD(runtime::WasmSpan,
                ext_crypto_ed25519_sign_version_1,
                (runtime::WasmSize key_type,
                 runtime::WasmPointer key,
                 runtime::WasmSpan msg_data),
                (override));

    MOCK_METHOD(runtime::WasmSize,
                ext_crypto_ed25519_verify_version_1,
                (runtime::WasmPointer sig_data,
                 runtime::WasmSpan msg,
                 runtime::WasmPointer pubkey_data),
                (override));

    MOCK_METHOD(runtime::WasmSpan,
                ext_crypto_sr25519_public_keys_version_1,
                (runtime::WasmSize key_type),
                (override));

    MOCK_METHOD(runtime::WasmPointer,
                ext_crypto_sr25519_generate_version_1,
                (runtime::WasmSize key_type, runtime::WasmSpan seed),
                (override));

    MOCK_METHOD(runtime::WasmSpan,
                ext_crypto_sr25519_sign_version_1,
                (runtime::WasmSize key_type,
                 runtime::WasmPointer key,
                 runtime::WasmSpan msg_data),
                (override));

    MOCK_METHOD(void, ext_crypto_start_batch_verify_version_1, (), (override));

    MOCK_METHOD(int32_t,
                ext_crypto_finish_batch_verify_version_1,
                (),
                (override));

    MOCK_METHOD(runtime::WasmSpan,
                ext_crypto_secp256k1_ecdsa_recover_version_1,
                (runtime::WasmPointer sig, runtime::WasmPointer msg),
                (override));

    MOCK_METHOD(runtime::WasmSpan,
                ext_crypto_secp256k1_ecdsa_recover_version_2,
                (runtime::WasmPointer sig, runtime::WasmPointer msg),
                (override));

    MOCK_METHOD(runtime::WasmSpan,
                ext_crypto_secp256k1_ecdsa_recover_compressed_version_1,
                (runtime::WasmPointer sig, runtime::WasmPointer msg),
                (override));

    MOCK_METHOD(runtime::WasmSpan,
                ext_crypto_secp256k1_ecdsa_recover_compressed_version_2,
                (runtime::WasmPointer sig, runtime::WasmPointer msg),
                (override));

    // ---------------------------- memory api v1 ----------------------------

    MOCK_METHOD(runtime::WasmPointer,
                ext_allocator_malloc_version_1,
                (runtime::WasmSize),
                (override));

    MOCK_METHOD(void,
                ext_allocator_free_version_1,
                (runtime::WasmPointer),
                (override));
  };

}  // namespace kagome::host_api

#endif  // KAGOME_TEST_CORE_RUNTIME_MOCK_EXTENSION_HPP_
