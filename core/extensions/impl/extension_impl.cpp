/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/extension_impl.hpp"

#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"

namespace kagome::extensions {

  ExtensionImpl::ExtensionImpl(
      const std::shared_ptr<runtime::WasmMemory> &memory,
      std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
      std::shared_ptr<storage::changes_trie::ChangesTracker> tracker)
      : memory_(memory),
        storage_provider_(std::move(storage_provider)),
        crypto_ext_(memory,
                    std::make_shared<crypto::SR25519ProviderImpl>(
                        std::make_shared<crypto::BoostRandomGenerator>()),
                    std::make_shared<crypto::ED25519ProviderImpl>(),
                    std::make_shared<crypto::Secp256k1ProviderImpl>(),
                    std::make_shared<crypto::HasherImpl>()),
        io_ext_(memory),
        memory_ext_(memory),
        storage_ext_(storage_provider_, memory_, std::move(tracker)) {
    std::make_shared<crypto::Secp256k1ProviderImpl>();
    BOOST_ASSERT(storage_provider_ != nullptr);
    BOOST_ASSERT(memory_ != nullptr);
  }

  std::shared_ptr<runtime::WasmMemory> ExtensionImpl::memory() const {
    return memory_;
  }

  // -------------------------Storage extensions--------------------------

  void ExtensionImpl::ext_clear_prefix(runtime::WasmPointer prefix_data,
                                       runtime::WasmSize prefix_length) {
    return storage_ext_.ext_clear_prefix(prefix_data, prefix_length);
  }

  void ExtensionImpl::ext_clear_storage(runtime::WasmPointer key_data,
                                        runtime::WasmSize key_length) {
    return storage_ext_.ext_clear_storage(key_data, key_length);
  }

  runtime::WasmSize ExtensionImpl::ext_exists_storage(
      runtime::WasmPointer key_data, runtime::WasmSize key_length) const {
    return storage_ext_.ext_exists_storage(key_data, key_length);
  }

  runtime::WasmPointer ExtensionImpl::ext_get_allocated_storage(
      runtime::WasmPointer key_data,
      runtime::WasmSize key_length,
      runtime::WasmPointer written) {
    return storage_ext_.ext_get_allocated_storage(
        key_data, key_length, written);
  }

  runtime::WasmSize ExtensionImpl::ext_get_storage_into(
      runtime::WasmPointer key_data,
      runtime::WasmSize key_length,
      runtime::WasmPointer value_data,
      runtime::WasmSize value_length,
      runtime::WasmSize value_offset) {
    return storage_ext_.ext_get_storage_into(
        key_data, key_length, value_data, value_length, value_offset);
  }

  void ExtensionImpl::ext_set_storage(runtime::WasmPointer key_data,
                                      runtime::WasmSize key_length,
                                      runtime::WasmPointer value_data,
                                      runtime::WasmSize value_length) {
    return storage_ext_.ext_set_storage(
        key_data, key_length, value_data, value_length);
  }

  void ExtensionImpl::ext_blake2_256_enumerated_trie_root(
      runtime::WasmPointer values_data,
      runtime::WasmPointer lens_data,
      runtime::WasmSize lens_length,
      runtime::WasmPointer result) {
    return storage_ext_.ext_blake2_256_enumerated_trie_root(
        values_data, lens_data, lens_length, result);
  }

  runtime::WasmSize ExtensionImpl::ext_storage_changes_root(
      runtime::WasmPointer parent_hash_data, runtime::WasmPointer result) {
    return storage_ext_.ext_storage_changes_root(parent_hash_data, result);
  }

  void ExtensionImpl::ext_storage_root(runtime::WasmPointer result) const {
    return storage_ext_.ext_storage_root(result);
  }

  // -------------------------Memory extensions--------------------------

  runtime::WasmPointer ExtensionImpl::ext_malloc(runtime::WasmSize size) {
    return memory_ext_.ext_malloc(size);
  }

  void ExtensionImpl::ext_free(runtime::WasmPointer ptr) {
    memory_ext_.ext_free(ptr);
  }

  /// I/O extensions
  void ExtensionImpl::ext_print_hex(runtime::WasmPointer data,
                                    runtime::WasmSize length) {
    io_ext_.ext_print_hex(data, length);
  }

  void ExtensionImpl::ext_print_num(uint64_t value) {
    io_ext_.ext_print_num(value);
  }

  void ExtensionImpl::ext_print_utf8(runtime::WasmPointer utf8_data,
                                     runtime::WasmSize utf8_length) {
    io_ext_.ext_print_utf8(utf8_data, utf8_length);
  }

  /// cryptographic extensions
  void ExtensionImpl::ext_blake2_128(runtime::WasmPointer data,
                                     runtime::WasmSize len,
                                     runtime::WasmPointer out) {
    crypto_ext_.ext_blake2_128(data, len, out);
  }

  void ExtensionImpl::ext_blake2_256(runtime::WasmPointer data,
                                     runtime::WasmSize len,
                                     runtime::WasmPointer out) {
    crypto_ext_.ext_blake2_256(data, len, out);
  }

  void ExtensionImpl::ext_keccak_256(runtime::WasmPointer data,
                                     runtime::WasmSize len,
                                     runtime::WasmPointer out) {
    crypto_ext_.ext_keccak_256(data, len, out);
  }

  runtime::WasmSize ExtensionImpl::ext_ed25519_verify(
      runtime::WasmPointer msg_data,
      runtime::WasmSize msg_len,
      runtime::WasmPointer sig_data,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_.ext_ed25519_verify(
        msg_data, msg_len, sig_data, pubkey_data);
  }

  runtime::WasmSize ExtensionImpl::ext_sr25519_verify(
      runtime::WasmPointer msg_data,
      runtime::WasmSize msg_len,
      runtime::WasmPointer sig_data,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_.ext_sr25519_verify(
        msg_data, msg_len, sig_data, pubkey_data);
  }

  void ExtensionImpl::ext_twox_64(runtime::WasmPointer data,
                                  runtime::WasmSize len,
                                  runtime::WasmPointer out) {
    crypto_ext_.ext_twox_64(data, len, out);
  }

  void ExtensionImpl::ext_twox_128(runtime::WasmPointer data,
                                   runtime::WasmSize len,
                                   runtime::WasmPointer out) {
    crypto_ext_.ext_twox_128(data, len, out);
  }

  void ExtensionImpl::ext_twox_256(runtime::WasmPointer data,
                                   runtime::WasmSize len,
                                   runtime::WasmPointer out) {
    crypto_ext_.ext_twox_256(data, len, out);
  }

  /// misc extensions
  uint64_t ExtensionImpl::ext_chain_id() const {
    return misc_ext_.ext_chain_id();
  }

  runtime::WasmSpan ExtensionImpl::ext_crypto_secp256k1_ecdsa_recover_v1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    return crypto_ext_.ext_crypto_secp256k1_ecdsa_recover_v1(sig, msg);
  }

  runtime::WasmSpan
  ExtensionImpl::ext_crypto_secp256k1_ecdsa_recover_compressed_v1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    return crypto_ext_.ext_crypto_secp256k1_ecdsa_recover_compressed_v1(sig,
                                                                        msg);
  }

}  // namespace kagome::extensions
