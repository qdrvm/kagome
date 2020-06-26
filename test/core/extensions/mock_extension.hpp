/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_RUNTIME_MOCK_EXTENSION_HPP_
#define KAGOME_TEST_CORE_RUNTIME_MOCK_EXTENSION_HPP_

#include "extensions/extension.hpp"

#include <gmock/gmock.h>

namespace kagome::extensions {

  class MockExtension : public Extension {
   public:
    MOCK_CONST_METHOD0(memory, std::shared_ptr<runtime::WasmMemory>());
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
    MOCK_METHOD4(ext_ed25519_verify,
                 runtime::WasmSize(runtime::WasmPointer msg_data,
                                   runtime::WasmSize msg_len,
                                   runtime::WasmPointer sig_data,
                                   runtime::WasmPointer pubkey_data));
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
    MOCK_METHOD2(ext_crypto_secp256k1_ecdsa_recover_v1,
                 runtime::WasmSpan(runtime::WasmPointer sig,
                                      runtime::WasmPointer msg));
    MOCK_METHOD2(ext_crypto_secp256k1_ecdsa_recover_compressed_v1,
                 runtime::WasmSpan(runtime::WasmPointer sig,
                                      runtime::WasmPointer msg));
  };

}  // namespace kagome::extensions

#endif  // KAGOME_TEST_CORE_RUNTIME_MOCK_EXTENSION_HPP_
