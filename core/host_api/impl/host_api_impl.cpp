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
      std::shared_ptr<const runtime::MemoryProvider> memory_provider,
      std::shared_ptr<const runtime::CoreApiProvider> core_provider,
      std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
      std::shared_ptr<storage::changes_trie::ChangesTracker> tracker,
      std::shared_ptr<const crypto::Sr25519Provider> sr25519_provider,
      std::shared_ptr<const crypto::Ed25519Provider> ed25519_provider,
      std::shared_ptr<const crypto::Secp256k1Provider> secp256k1_provider,
      std::shared_ptr<const crypto::Hasher> hasher,
      std::shared_ptr<crypto::CryptoStore> crypto_store,
      std::shared_ptr<const crypto::Bip39Provider> bip39_provider)
      : memory_provider_(memory_provider),
        storage_provider_(std::move(storage_provider)),
        crypto_ext_{
            std::make_shared<CryptoExtension>(memory_provider_,
                                              std::move(sr25519_provider),
                                              std::move(ed25519_provider),
                                              std::move(secp256k1_provider),
                                              hasher,
                                              std::move(crypto_store),
                                              std::move(bip39_provider))},
        io_ext_(memory_provider_),
        memory_ext_(memory_provider_),
        misc_ext_{DEFAULT_CHAIN_ID, hasher, memory_provider_, std::move(core_provider)},
        storage_ext_(storage_provider_, memory_provider_, std::move(tracker)) {
    BOOST_ASSERT(storage_provider_ != nullptr);
    BOOST_ASSERT(memory_provider_ != nullptr);
  }

  void HostApiImpl::reset() {
    memory_ext_.reset();
    crypto_ext_->reset();
    storage_ext_.reset();
  }

  runtime::WasmSpan HostApiImpl::ext_storage_read_version_1(
      runtime::WasmSpan key,
      runtime::WasmSpan value_out,
      runtime::WasmOffset offset) {
    return storage_ext_.ext_storage_read_version_1(key, value_out, offset);
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

  // ------------------------Memory extensions v1-------------------------
  runtime::WasmPointer HostApiImpl::ext_allocator_malloc_version_1(
      runtime::WasmSize size) {
    return memory_ext_.ext_allocator_malloc_version_1(size);
  }

  void HostApiImpl::ext_allocator_free_version_1(runtime::WasmPointer ptr) {
    return memory_ext_.ext_allocator_free_version_1(ptr);
  }

  void HostApiImpl::ext_logging_log_version_1(runtime::WasmEnum level,
                                              runtime::WasmSpan target,
                                              runtime::WasmSpan message) {
    io_ext_.ext_logging_log_version_1(level, target, message);
  }

  runtime::WasmEnum HostApiImpl::ext_logging_max_level_version_1() {
    return io_ext_.ext_logging_max_level_version_1();
  }

  /// Crypto extensions v1

  void HostApiImpl::ext_crypto_start_batch_verify_version_1() {
    return crypto_ext_->ext_crypto_start_batch_verify_version_1();
  }

  int32_t HostApiImpl::ext_crypto_finish_batch_verify_version_1() {
    return crypto_ext_->ext_crypto_finish_batch_verify_version_1();
  }

  runtime::WasmSpan HostApiImpl::ext_crypto_ed25519_public_keys_version_1(
      runtime::WasmSize key_type) {
    return crypto_ext_->ext_crypto_ed25519_public_keys_version_1(key_type);
  }

  runtime::WasmPointer HostApiImpl::ext_crypto_ed25519_generate_version_1(
      runtime::WasmSize key_type, runtime::WasmSpan seed) {
    return crypto_ext_->ext_crypto_ed25519_generate_version_1(key_type, seed);
  }

  runtime::WasmSpan HostApiImpl::ext_crypto_ed25519_sign_version_1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg_data) {
    return crypto_ext_->ext_crypto_ed25519_sign_version_1(
        key_type, key, msg_data);
  }

  runtime::WasmSize HostApiImpl::ext_crypto_ed25519_verify_version_1(
      runtime::WasmPointer sig_data,
      runtime::WasmSpan msg,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_->ext_crypto_ed25519_verify_version_1(
        sig_data, msg, pubkey_data);
  }

  runtime::WasmSpan HostApiImpl::ext_crypto_sr25519_public_keys_version_1(
      runtime::WasmSize key_type) {
    return crypto_ext_->ext_crypto_sr25519_public_keys_version_1(key_type);
  }

  runtime::WasmPointer HostApiImpl::ext_crypto_sr25519_generate_version_1(
      runtime::WasmSize key_type, runtime::WasmSpan seed) {
    return crypto_ext_->ext_crypto_sr25519_generate_version_1(key_type, seed);
  }

  runtime::WasmSpan HostApiImpl::ext_crypto_sr25519_sign_version_1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg_data) {
    return crypto_ext_->ext_crypto_sr25519_sign_version_1(
        key_type, key, msg_data);
  }

  int32_t HostApiImpl::ext_crypto_sr25519_verify_version_1(
      runtime::WasmPointer sig_data,
      runtime::WasmSpan msg,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_->ext_crypto_sr25519_verify_version_1(
        sig_data, msg, pubkey_data);
  }

  int32_t HostApiImpl::ext_crypto_sr25519_verify_version_2(
      runtime::WasmPointer sig_data,
      runtime::WasmSpan msg,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_->ext_crypto_sr25519_verify_version_2(
        sig_data, msg, pubkey_data);
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

  runtime::WasmSpan HostApiImpl::ext_misc_runtime_version_version_1(
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

  runtime::WasmSpan HostApiImpl::ext_crypto_secp256k1_ecdsa_recover_version_1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    return crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_version_1(sig, msg);
  }

  runtime::WasmSpan
  HostApiImpl::ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    return crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(
        sig, msg);
  }
}  // namespace kagome::host_api
