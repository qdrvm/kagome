/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HOST_API_IMPL_HPP
#define KAGOME_HOST_API_IMPL_HPP

#include "host_api/host_api.hpp"
#include "host_api/impl/crypto_extension.hpp"
#include "host_api/impl/io_extension.hpp"
#include "host_api/impl/memory_extension.hpp"
#include "host_api/impl/misc_extension.hpp"
#include "host_api/impl/storage_extension.hpp"

namespace kagome::runtime {
  class Core;
  class MemoryProvider;
}  // namespace kagome::runtime

namespace kagome::host_api {
  /**
   * Fair implementation of the extensions interface
   */
  class  HostApiImpl : public HostApi {
   public:
    HostApiImpl() = delete;
    HostApiImpl(
        std::shared_ptr<const runtime::MemoryProvider> memory_provider,
        std::shared_ptr<const runtime::CoreApiFactory> core_provider,
        std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
        std::shared_ptr<storage::changes_trie::ChangesTracker> tracker,
        std::shared_ptr<const crypto::Sr25519Provider> sr25519_provider,
        std::shared_ptr<const crypto::Ed25519Provider> ed25519_provider,
        std::shared_ptr<const crypto::Secp256k1Provider> secp256k1_provider,
        std::shared_ptr<const crypto::Hasher> hasher,
        std::shared_ptr<crypto::CryptoStore> crypto_store,
        std::shared_ptr<const crypto::Bip39Provider> bip39_provider);

    ~HostApiImpl() override = default;

    void reset() override;

    // ------------------------ Storage extensions v1 ------------------------

    runtime::WasmSpan ext_storage_read_version_1(
        runtime::WasmSpan key,
        runtime::WasmSpan value_out,
        runtime::WasmOffset offset) override;

    void ext_storage_set_version_1(runtime::WasmSpan key,
                                   runtime::WasmSpan value) override;

    runtime::WasmSpan ext_storage_get_version_1(runtime::WasmSpan key) override;

    void ext_storage_clear_version_1(runtime::WasmSpan key_data) override;

    runtime::WasmSize ext_storage_exists_version_1(
        runtime::WasmSpan key_data) const override;

    void ext_storage_clear_prefix_version_1(runtime::WasmSpan prefix) override;

    runtime::WasmSpan ext_storage_root_version_1() override;

    runtime::WasmSpan ext_storage_changes_root_version_1(
        runtime::WasmSpan parent_hash) override;

    runtime::WasmSpan ext_storage_next_key_version_1(
        runtime::WasmSpan key) const override;

    void ext_storage_append_version_1(runtime::WasmSpan key,
                                      runtime::WasmSpan value) const override;

    runtime::WasmPointer ext_trie_blake2_256_root_version_1(
        runtime::WasmSpan values_data) override;

    runtime::WasmPointer ext_trie_blake2_256_ordered_root_version_1(
        runtime::WasmSpan values_data) override;

    // ------------------------Memory extensions v1-------------------------
    runtime::WasmPointer ext_allocator_malloc_version_1(
        runtime::WasmSize size) override;

    void ext_allocator_free_version_1(runtime::WasmPointer ptr) override;

    // -------------------------Crypto extensions v1---------------------

    void ext_crypto_start_batch_verify_version_1() override;

    [[nodiscard]] int32_t ext_crypto_finish_batch_verify_version_1() override;

    runtime::WasmSpan ext_crypto_secp256k1_ecdsa_recover_version_1(
        runtime::WasmPointer sig, runtime::WasmPointer msg) override;

    runtime::WasmSpan ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(
        runtime::WasmPointer sig, runtime::WasmPointer msg) override;

    runtime::WasmSpan ext_crypto_ed25519_public_keys_version_1(
        runtime::WasmSize key_type) override;

    runtime::WasmPointer ext_crypto_ed25519_generate_version_1(
        runtime::WasmSize key_type, runtime::WasmSpan seed) override;

    runtime::WasmSpan ext_crypto_ed25519_sign_version_1(
        runtime::WasmSize key_type,
        runtime::WasmPointer key,
        runtime::WasmSpan msg_data) override;

    runtime::WasmSize ext_crypto_ed25519_verify_version_1(
        runtime::WasmPointer sig_data,
        runtime::WasmSpan msg,
        runtime::WasmPointer pubkey_data) override;

    runtime::WasmSpan ext_crypto_sr25519_public_keys_version_1(
        runtime::WasmSize key_type) override;

    runtime::WasmPointer ext_crypto_sr25519_generate_version_1(
        runtime::WasmSize key_type, runtime::WasmSpan seed) override;

    runtime::WasmSpan ext_crypto_sr25519_sign_version_1(
        runtime::WasmSize key_type,
        runtime::WasmPointer key,
        runtime::WasmSpan msg_data) override;

    int32_t ext_crypto_sr25519_verify_version_1(
        runtime::WasmPointer sig_data,
        runtime::WasmSpan msg,
        runtime::WasmPointer pubkey_data) override;

    int32_t ext_crypto_sr25519_verify_version_2(runtime::WasmPointer,
                                                runtime::WasmSpan,
                                                runtime::WasmPointer) override;

    // ------------------------- Hashing extension/crypto ---------------

    runtime::WasmPointer ext_hashing_keccak_256_version_1(
        runtime::WasmSpan data) override;

    runtime::WasmPointer ext_hashing_sha2_256_version_1(
        runtime::WasmSpan data) override;

    runtime::WasmPointer ext_hashing_blake2_128_version_1(
        runtime::WasmSpan data) override;

    runtime::WasmPointer ext_hashing_blake2_256_version_1(
        runtime::WasmSpan data) override;

    runtime::WasmPointer ext_hashing_twox_64_version_1(
        runtime::WasmSpan data) override;

    runtime::WasmPointer ext_hashing_twox_128_version_1(
        runtime::WasmSpan data) override;

    runtime::WasmPointer ext_hashing_twox_256_version_1(
        runtime::WasmSpan data) override;

    // -------------------------Misc extensions--------------------------

    void ext_logging_log_version_1(runtime::WasmEnum level,
                                   runtime::WasmSpan target,
                                   runtime::WasmSpan message) override;

    runtime::WasmEnum ext_logging_max_level_version_1() override;

    runtime::WasmSpan ext_misc_runtime_version_version_1(
        runtime::WasmSpan data) const override;

    void ext_misc_print_hex_version_1(runtime::WasmSpan data) const override;

    void ext_misc_print_num_version_1(uint64_t value) const override;

    void ext_misc_print_utf8_version_1(runtime::WasmSpan data) const override;

   private:
    static constexpr uint64_t DEFAULT_CHAIN_ID = 42;

    std::shared_ptr<const runtime::MemoryProvider> memory_provider_;
    std::shared_ptr<runtime::TrieStorageProvider> storage_provider_;

    std::shared_ptr<CryptoExtension> crypto_ext_;
    IOExtension io_ext_;
    MemoryExtension memory_ext_;
    MiscExtension misc_ext_;
    StorageExtension storage_ext_;
  };
}  // namespace kagome::host_api

#endif  // KAGOME_HOST_API_IMPL_HPP
