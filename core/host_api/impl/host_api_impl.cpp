/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/host_api_impl.hpp"

#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/crypto_store/crypto_store_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"

namespace kagome::host_api {

  HostApiImpl::HostApiImpl(
      const std::shared_ptr<runtime::WasmMemory> &memory,
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
      std::shared_ptr<crypto::Bip39Provider> bip39_provider)
      : memory_(memory),
        storage_provider_(std::move(storage_provider)),
        crypto_ext_{
            std::make_shared<CryptoExtension>(memory,
                                              std::move(sr25519_provider),
                                              std::move(ed25519_provider),
                                              std::move(secp256k1_provider),
                                              std::move(hasher),
                                              std::move(crypto_store),
                                              std::move(bip39_provider))},
        io_ext_(memory),
        memory_ext_(memory),
        misc_ext_{DEFAULT_CHAIN_ID,
                  std::move(core_factory),
                  std::move(runtime_env_factory),
                  memory},
        storage_ext_(storage_provider_, memory_, std::move(tracker)) {
    BOOST_ASSERT(storage_provider_ != nullptr);
    BOOST_ASSERT(memory_ != nullptr);
  }

  std::shared_ptr<runtime::WasmMemory> HostApiImpl::memory() const {
    return memory_;
  }

  void HostApiImpl::reset() {
    memory_ext_.reset();
    crypto_ext_->reset();
    storage_ext_.reset();
  }

  // -------------------------Storage extensions--------------------------

  void HostApiImpl::ext_clear_prefix(runtime::WasmPointer prefix_data,
                                     runtime::WasmSize prefix_length) {
    storage_ext_.ext_clear_prefix(prefix_data, prefix_length);
  }

  void HostApiImpl::ext_clear_storage(runtime::WasmPointer key_data,
                                      runtime::WasmSize key_length) {
    return storage_ext_.ext_clear_storage(key_data, key_length);
  }

  runtime::WasmSize HostApiImpl::ext_exists_storage(
      runtime::WasmPointer key_data, runtime::WasmSize key_length) const {
    return storage_ext_.ext_exists_storage(key_data, key_length);
  }

  runtime::WasmPointer HostApiImpl::ext_get_allocated_storage(
      runtime::WasmPointer key_data,
      runtime::WasmSize key_length,
      runtime::WasmPointer written) {
    return storage_ext_.ext_get_allocated_storage(
        key_data, key_length, written);
  }

  runtime::WasmSize HostApiImpl::ext_get_storage_into(
      runtime::WasmPointer key_data,
      runtime::WasmSize key_length,
      runtime::WasmPointer value_data,
      runtime::WasmSize value_length,
      runtime::WasmSize value_offset) {
    return storage_ext_.ext_get_storage_into(
        key_data, key_length, value_data, value_length, value_offset);
  }

  runtime::WasmSpan HostApiImpl::ext_storage_read_version_1(
      runtime::WasmSpan key,
      runtime::WasmSpan value_out,
      runtime::WasmOffset offset) {
    return storage_ext_.ext_storage_read_version_1(key, value_out, offset);
  }

  void HostApiImpl::ext_set_storage(runtime::WasmPointer key_data,
                                    runtime::WasmSize key_length,
                                    runtime::WasmPointer value_data,
                                    runtime::WasmSize value_length) {
    return storage_ext_.ext_set_storage(
        key_data, key_length, value_data, value_length);
  }

  void HostApiImpl::ext_blake2_256_enumerated_trie_root(
      runtime::WasmPointer values_data,
      runtime::WasmPointer lens_data,
      runtime::WasmSize lens_length,
      runtime::WasmPointer result) {
    return storage_ext_.ext_blake2_256_enumerated_trie_root(
        values_data, lens_data, lens_length, result);
  }

  runtime::WasmSize HostApiImpl::ext_storage_changes_root(
      runtime::WasmPointer parent_hash_data, runtime::WasmPointer result) {
    return storage_ext_.ext_storage_changes_root(parent_hash_data, result);
  }

  void HostApiImpl::ext_storage_root(runtime::WasmPointer result) const {
    return storage_ext_.ext_storage_root(result);
  }

  void HostApiImpl::ext_storage_start_transaction() {
    return storage_ext_.ext_storage_start_transaction();
  }

  void HostApiImpl::ext_storage_rollback_transaction() {
    return storage_ext_.ext_storage_rollback_transaction();
  }

  void HostApiImpl::ext_storage_commit_transaction() {
    return storage_ext_.ext_storage_commit_transaction();
  }

  runtime::WasmSpan HostApiImpl::ext_storage_next_key_version_1(
      runtime::WasmSpan key) const {
    return storage_ext_.ext_storage_next_key_version_1(key);
  }

  void HostApiImpl::ext_storage_append_version_1(
      runtime::WasmSpan key, runtime::WasmSpan value) const {
    return storage_ext_.ext_storage_append_version_1(key, value);
  }

  void HostApiImpl::ext_storage_set_version_1(runtime::WasmSpan key,
                                              runtime::WasmSpan value) {
    return storage_ext_.ext_storage_set_version_1(key, value);
  }

  runtime::WasmSpan HostApiImpl::ext_storage_get_version_1(
      runtime::WasmSpan key) {
    return storage_ext_.ext_storage_get_version_1(key);
  }

  void HostApiImpl::ext_storage_clear_version_1(runtime::WasmSpan key_data) {
    return storage_ext_.ext_storage_clear_version_1(key_data);
  }

  runtime::WasmSize HostApiImpl::ext_storage_exists_version_1(
      runtime::WasmSpan key_data) const {
    return storage_ext_.ext_storage_exists_version_1(key_data);
  }

  void HostApiImpl::ext_storage_clear_prefix_version_1(
      runtime::WasmSpan prefix) {
    return storage_ext_.ext_storage_clear_prefix_version_1(prefix);
  }

  runtime::WasmSpan HostApiImpl::ext_storage_clear_prefix_version_2(
      runtime::WasmSpan prefix, runtime::WasmSpan limit) {
    return storage_ext_.ext_storage_clear_prefix_version_2(prefix, limit);
  }

  runtime::WasmSpan HostApiImpl::ext_storage_root_version_1() {
    return storage_ext_.ext_storage_root_version_1();
  }

  runtime::WasmSpan HostApiImpl::ext_storage_changes_root_version_1(
      runtime::WasmSpan parent_hash) {
    return storage_ext_.ext_storage_changes_root_version_1(parent_hash);
  }

  runtime::WasmPointer HostApiImpl::ext_trie_blake2_256_root_version_1(
      runtime::WasmSpan values_data) {
    return storage_ext_.ext_trie_blake2_256_root_version_1(values_data);
  }

  runtime::WasmPointer HostApiImpl::ext_trie_blake2_256_ordered_root_version_1(
      runtime::WasmSpan values_data) {
    return storage_ext_.ext_trie_blake2_256_ordered_root_version_1(values_data);
  }

  // -------------------------Memory extensions--------------------------

  runtime::WasmPointer HostApiImpl::ext_malloc(runtime::WasmSize size) {
    return memory_ext_.ext_malloc(size);
  }

  void HostApiImpl::ext_free(runtime::WasmPointer ptr) {
    memory_ext_.ext_free(ptr);
  }

  // ------------------------Memory extensions v1-------------------------
  runtime::WasmPointer HostApiImpl::ext_allocator_malloc_version_1(
      runtime::WasmSize size) {
    return memory_ext_.ext_allocator_malloc_version_1(size);
  }

  void HostApiImpl::ext_allocator_free_version_1(runtime::WasmPointer ptr) {
    return memory_ext_.ext_free(ptr);
  }

  /// I/O extensions
  void HostApiImpl::ext_print_hex(runtime::WasmPointer data,
                                  runtime::WasmSize length) {
    io_ext_.ext_print_hex(data, length);
  }

  void HostApiImpl::ext_logging_log_version_1(runtime::WasmEnum level,
                                              runtime::WasmSpan target,
                                              runtime::WasmSpan message) {
    io_ext_.ext_logging_log_version_1(level, target, message);
  }

  runtime::WasmEnum HostApiImpl::ext_logging_max_level_version_1() {
    return io_ext_.ext_logging_max_level_version_1();
  }

  void HostApiImpl::ext_print_num(uint64_t value) {
    io_ext_.ext_print_num(value);
  }

  void HostApiImpl::ext_print_utf8(runtime::WasmPointer utf8_data,
                                   runtime::WasmSize utf8_length) {
    io_ext_.ext_print_utf8(utf8_data, utf8_length);
  }

  /// cryptographic extensions
  void HostApiImpl::ext_blake2_128(runtime::WasmPointer data,
                                   runtime::WasmSize len,
                                   runtime::WasmPointer out) {
    crypto_ext_->ext_blake2_128(data, len, out);
  }

  void HostApiImpl::ext_blake2_256(runtime::WasmPointer data,
                                   runtime::WasmSize len,
                                   runtime::WasmPointer out) {
    crypto_ext_->ext_blake2_256(data, len, out);
  }

  void HostApiImpl::ext_keccak_256(runtime::WasmPointer data,
                                   runtime::WasmSize len,
                                   runtime::WasmPointer out) {
    crypto_ext_->ext_keccak_256(data, len, out);
  }

  void HostApiImpl::ext_start_batch_verify() {
    crypto_ext_->ext_start_batch_verify();
  }

  runtime::WasmSize HostApiImpl::ext_finish_batch_verify() {
    return crypto_ext_->ext_finish_batch_verify();
  }

  runtime::WasmSize HostApiImpl::ext_ed25519_verify(
      runtime::WasmPointer msg_data,
      runtime::WasmSize msg_len,
      runtime::WasmPointer sig_data,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_->ext_ed25519_verify(
        msg_data, msg_len, sig_data, pubkey_data);
  }

  runtime::WasmSize HostApiImpl::ext_sr25519_verify(
      runtime::WasmPointer msg_data,
      runtime::WasmSize msg_len,
      runtime::WasmPointer sig_data,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_->ext_sr25519_verify(
        msg_data, msg_len, sig_data, pubkey_data);
  }

  void HostApiImpl::ext_twox_64(runtime::WasmPointer data,
                                runtime::WasmSize len,
                                runtime::WasmPointer out) {
    crypto_ext_->ext_twox_64(data, len, out);
  }

  void HostApiImpl::ext_twox_128(runtime::WasmPointer data,
                                 runtime::WasmSize len,
                                 runtime::WasmPointer out) {
    crypto_ext_->ext_twox_128(data, len, out);
  }

  void HostApiImpl::ext_twox_256(runtime::WasmPointer data,
                                 runtime::WasmSize len,
                                 runtime::WasmPointer out) {
    crypto_ext_->ext_twox_256(data, len, out);
  }

  /// Crypto extensions v1

  runtime::WasmSpan HostApiImpl::ext_ed25519_public_keys_v1(
      runtime::WasmSize key_type) {
    return crypto_ext_->ext_ed25519_public_keys_v1(key_type);
  }

  runtime::WasmPointer HostApiImpl::ext_ed25519_generate_v1(
      runtime::WasmSize key_type, runtime::WasmSpan seed) {
    return crypto_ext_->ext_ed25519_generate_v1(key_type, seed);
  }

  runtime::WasmSpan HostApiImpl::ext_ed25519_sign_v1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg_data) {
    return crypto_ext_->ext_ed25519_sign_v1(key_type, key, msg_data);
  }

  runtime::WasmSize HostApiImpl::ext_ed25519_verify_v1(
      runtime::WasmPointer sig_data,
      runtime::WasmSpan msg,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_->ext_ed25519_verify_v1(sig_data, msg, pubkey_data);
  }

  runtime::WasmSpan HostApiImpl::ext_sr25519_public_keys_v1(
      runtime::WasmSize key_type) {
    return crypto_ext_->ext_sr25519_public_keys_v1(key_type);
  }

  runtime::WasmPointer HostApiImpl::ext_sr25519_generate_v1(
      runtime::WasmSize key_type, runtime::WasmSpan seed) {
    return crypto_ext_->ext_sr25519_generate_v1(key_type, seed);
  }

  runtime::WasmSpan HostApiImpl::ext_sr25519_sign_v1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg_data) {
    return crypto_ext_->ext_sr25519_sign_v1(key_type, key, msg_data);
  }

  runtime::WasmSize HostApiImpl::ext_sr25519_verify_v1(
      runtime::WasmPointer sig_data,
      runtime::WasmSpan msg,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_->ext_sr25519_verify_v1(sig_data, msg, pubkey_data);
  }

  // ------------------------- Hashing extension/crypto ---------------

  runtime::WasmPointer HostApiImpl::ext_hashing_keccak_256_version_1(
      runtime::WasmSpan data) {
    return crypto_ext_->ext_hashing_keccak_256_version_1(data);
  }

  runtime::WasmPointer HostApiImpl::ext_hashing_sha2_256_version_1(
      runtime::WasmSpan data) {
    return crypto_ext_->ext_hashing_sha2_256_version_1(data);
  }

  runtime::WasmPointer HostApiImpl::ext_hashing_blake2_128_version_1(
      runtime::WasmSpan data) {
    return crypto_ext_->ext_hashing_blake2_128_version_1(data);
  }

  runtime::WasmPointer HostApiImpl::ext_hashing_blake2_256_version_1(
      runtime::WasmSpan data) {
    return crypto_ext_->ext_hashing_blake2_256_version_1(data);
  }

  runtime::WasmPointer HostApiImpl::ext_hashing_twox_64_version_1(
      runtime::WasmSpan data) {
    return crypto_ext_->ext_hashing_twox_64_version_1(data);
  }

  runtime::WasmPointer HostApiImpl::ext_hashing_twox_128_version_1(
      runtime::WasmSpan data) {
    return crypto_ext_->ext_hashing_twox_128_version_1(data);
  }

  runtime::WasmPointer HostApiImpl::ext_hashing_twox_256_version_1(
      runtime::WasmSpan data) {
    return crypto_ext_->ext_hashing_twox_256_version_1(data);
  }

  /// misc extensions
  uint64_t HostApiImpl::ext_chain_id() const {
    return misc_ext_.ext_chain_id();
  }

  runtime::WasmResult HostApiImpl::ext_misc_runtime_version_version_1(
      runtime::WasmSpan data) const {
    return misc_ext_.ext_misc_runtime_version_version_1(data);
  }

  void HostApiImpl::ext_misc_print_hex_version_1(runtime::WasmSpan data) const {
    return misc_ext_.ext_misc_print_hex_version_1(data);
  }

  void HostApiImpl::ext_misc_print_num_version_1(uint64_t value) const {
    return misc_ext_.ext_misc_print_num_version_1(value);
  }

  void HostApiImpl::ext_misc_print_utf8_version_1(
      runtime::WasmSpan data) const {
    return misc_ext_.ext_misc_print_utf8_version_1(data);
  }

  runtime::WasmSpan HostApiImpl::ext_crypto_secp256k1_ecdsa_recover_v1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    return crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_v1(sig, msg);
  }

  runtime::WasmSpan
  HostApiImpl::ext_crypto_secp256k1_ecdsa_recover_compressed_v1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    return crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_compressed_v1(sig,
                                                                         msg);
  }
}  // namespace kagome::host_api
