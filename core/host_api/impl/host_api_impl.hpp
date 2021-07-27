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

namespace kagome::runtime::binaryen {
  class CoreFactory;
  class RuntimeEnvironmentFactory;
}

namespace kagome::host_api {
  /**
   * Fair implementation of the extensions interface
   */
  class HostApiImpl : public HostApi {
   public:
    HostApiImpl() = delete;
    HostApiImpl(const std::shared_ptr<runtime::WasmMemory> &memory,
                std::shared_ptr<runtime::binaryen::CoreFactory> core_factory,
                std::shared_ptr<runtime::binaryen::RuntimeEnvironmentFactory>
                    runtime_env_factory,
                std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
                std::shared_ptr<storage::changes_trie::ChangesTracker> tracker,
                std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
                std::shared_ptr<crypto::Ed25519Provider> ed25519_provider,
                std::shared_ptr<crypto::Secp256k1Provider> secp256k1_provider,
                std::shared_ptr<crypto::Hasher> hasher,
                std::shared_ptr<crypto::CryptoStore> crypto_store,
                std::shared_ptr<crypto::Bip39Provider> bip39_provider);

    ~HostApiImpl() override = default;

    std::shared_ptr<runtime::WasmMemory> memory() const override;
    void reset() override;

    // -------------------------Storage extensions--------------------------

    void ext_clear_prefix(runtime::WasmPointer prefix_data,
                          runtime::WasmSize prefix_length) override;

    void ext_clear_storage(runtime::WasmPointer key_data,
                           runtime::WasmSize key_length) override;

    runtime::WasmSize ext_exists_storage(
        runtime::WasmPointer key_data,
        runtime::WasmSize key_length) const override;

    runtime::WasmPointer ext_get_allocated_storage(
        runtime::WasmPointer key_data,
        runtime::WasmSize key_length,
        runtime::WasmPointer len_ptr) override;

    runtime::WasmSize ext_get_storage_into(
        runtime::WasmPointer key_data,
        runtime::WasmSize key_length,
        runtime::WasmPointer value_data,
        runtime::WasmSize value_length,
        runtime::WasmSize value_offset) override;

    runtime::WasmSpan ext_storage_read_version_1(
        runtime::WasmSpan key,
        runtime::WasmSpan value_out,
        runtime::WasmOffset offset) override;

    void ext_set_storage(runtime::WasmPointer key_data,
                         runtime::WasmSize key_length,
                         runtime::WasmPointer value_data,
                         runtime::WasmSize value_length) override;

    void ext_blake2_256_enumerated_trie_root(
        runtime::WasmPointer values_data,
        runtime::WasmPointer lens_data,
        runtime::WasmSize lens_length,
        runtime::WasmPointer result) override;

    runtime::WasmSize ext_storage_changes_root(
        runtime::WasmPointer parent_hash_data,
        runtime::WasmPointer result) override;

    void ext_storage_root(runtime::WasmPointer result) const override;

    void ext_storage_start_transaction() override;

    void ext_storage_rollback_transaction() override;

    void ext_storage_commit_transaction() override;

    // ------------------------ Storage extensions v1 ------------------------

    void ext_storage_set_version_1(runtime::WasmSpan key,
                                   runtime::WasmSpan value) override;

    runtime::WasmSpan ext_storage_get_version_1(runtime::WasmSpan key) override;

    void ext_storage_clear_version_1(runtime::WasmSpan key_data) override;

    runtime::WasmSize ext_storage_exists_version_1(
        runtime::WasmSpan key_data) const override;

    void ext_storage_clear_prefix_version_1(runtime::WasmSpan prefix) override;

    runtime::WasmSpan ext_storage_clear_prefix_version_2(runtime::WasmSpan prefix,
                                            runtime::WasmSpan limit) override;

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

    // -------------------------Memory extensions--------------------------
    runtime::WasmPointer ext_malloc(runtime::WasmSize size) override;

    void ext_free(runtime::WasmPointer ptr) override;

    // ------------------------Memory extensions v1-------------------------
    runtime::WasmPointer ext_allocator_malloc_version_1(
        runtime::WasmSize size) override;

    void ext_allocator_free_version_1(runtime::WasmPointer ptr) override;

    // -------------------------I/O extensions--------------------------
    void ext_print_hex(runtime::WasmPointer data,
                       runtime::WasmSize length) override;

    void ext_logging_log_version_1(runtime::WasmEnum level,
                                   runtime::WasmSpan target,
                                   runtime::WasmSpan message) override;

    runtime::WasmEnum ext_logging_max_level_version_1() override;

    void ext_print_num(uint64_t value) override;

    void ext_print_utf8(runtime::WasmPointer utf8_data,
                        runtime::WasmSize utf8_length) override;

    // -------------------------Cryptographic extensions----------------------
    void ext_blake2_128(runtime::WasmPointer data,
                        runtime::WasmSize len,
                        runtime::WasmPointer out) override;

    void ext_blake2_256(runtime::WasmPointer data,
                        runtime::WasmSize len,
                        runtime::WasmPointer out) override;

    void ext_keccak_256(runtime::WasmPointer data,
                        runtime::WasmSize len,
                        runtime::WasmPointer out) override;

    void ext_start_batch_verify() override;

    runtime::WasmSize ext_finish_batch_verify() override;

    runtime::WasmSize ext_ed25519_verify(
        runtime::WasmPointer msg_data,
        runtime::WasmSize msg_len,
        runtime::WasmPointer sig_data,
        runtime::WasmPointer pubkey_data) override;

    runtime::WasmSize ext_sr25519_verify(
        runtime::WasmPointer msg_data,
        runtime::WasmSize msg_len,
        runtime::WasmPointer sig_data,
        runtime::WasmPointer pubkey_data) override;

    void ext_twox_64(runtime::WasmPointer data,
                     runtime::WasmSize len,
                     runtime::WasmPointer out) override;

    void ext_twox_128(runtime::WasmPointer data,
                      runtime::WasmSize len,
                      runtime::WasmPointer out) override;

    void ext_twox_256(runtime::WasmPointer data,
                      runtime::WasmSize len,
                      runtime::WasmPointer out) override;

    // -------------------------Crypto extensions v1---------------------

    runtime::WasmSpan ext_ed25519_public_keys_v1(
        runtime::WasmSize key_type) override;

    runtime::WasmPointer ext_ed25519_generate_v1(
        runtime::WasmSize key_type, runtime::WasmSpan seed) override;

    runtime::WasmSpan ext_ed25519_sign_v1(runtime::WasmSize key_type,
                                          runtime::WasmPointer key,
                                          runtime::WasmSpan msg_data) override;

    runtime::WasmSize ext_ed25519_verify_v1(
        runtime::WasmPointer sig_data,
        runtime::WasmSpan msg,
        runtime::WasmPointer pubkey_data) override;

    runtime::WasmSpan ext_sr25519_public_keys_v1(
        runtime::WasmSize key_type) override;

    runtime::WasmPointer ext_sr25519_generate_v1(
        runtime::WasmSize key_type, runtime::WasmSpan seed) override;

    runtime::WasmSpan ext_sr25519_sign_v1(runtime::WasmSize key_type,
                                          runtime::WasmPointer key,
                                          runtime::WasmSpan msg_data) override;

    runtime::WasmSize ext_sr25519_verify_v1(
        runtime::WasmPointer sig_data,
        runtime::WasmSpan msg,
        runtime::WasmPointer pubkey_data) override;

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

    uint64_t ext_chain_id() const override;

    runtime::WasmResult ext_misc_runtime_version_version_1(
        runtime::WasmSpan data) const override;

    void ext_misc_print_hex_version_1(runtime::WasmSpan data) const override;

    void ext_misc_print_num_version_1(uint64_t value) const override;

    void ext_misc_print_utf8_version_1(runtime::WasmSpan data) const override;

    runtime::WasmSpan ext_crypto_secp256k1_ecdsa_recover_v1(
        runtime::WasmPointer sig, runtime::WasmPointer msg) override;

    runtime::WasmSpan ext_crypto_secp256k1_ecdsa_recover_compressed_v1(
        runtime::WasmPointer sig, runtime::WasmPointer msg) override;

   private:
    static constexpr uint64_t DEFAULT_CHAIN_ID = 42;

    std::shared_ptr<runtime::WasmMemory> memory_;
    std::shared_ptr<runtime::TrieStorageProvider> storage_provider_;

    std::shared_ptr<CryptoExtension> crypto_ext_;
    IOExtension io_ext_;
    MemoryExtension memory_ext_;
    MiscExtension misc_ext_;
    StorageExtension storage_ext_;
  };
}  // namespace kagome::host_api

#endif  // KAGOME_HOST_API_IMPL_HPP
