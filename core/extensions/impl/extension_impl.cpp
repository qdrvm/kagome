/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/extension_impl.hpp"

#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "crypto/typed_key_storage/typed_key_storage_impl.hpp"

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
                    std::make_shared<crypto::HasherImpl>(),
                    std::make_shared<crypto::storage::TypedKeyStorageImpl>(),
                    std::make_shared<crypto::Bip39ProviderImpl>(
                        std::make_shared<crypto::Pbkdf2ProviderImpl>())),
        io_ext_(memory),
        memory_ext_(memory),
        storage_ext_(storage_provider_, memory_, std::move(tracker)) {
    BOOST_ASSERT(storage_provider_ != nullptr);
    BOOST_ASSERT(memory_ != nullptr);
  }

  std::shared_ptr<runtime::WasmMemory> ExtensionImpl::memory() const {
    return memory_;
  }

  // -------------------------Storage extensions--------------------------

  void ExtensionImpl::ext_clear_prefix(runtime::WasmPointer prefix_data,
                                       runtime::SizeType prefix_length) {
    return storage_ext_.ext_clear_prefix(prefix_data, prefix_length);
  }

  void ExtensionImpl::ext_clear_storage(runtime::WasmPointer key_data,
                                        runtime::SizeType key_length) {
    return storage_ext_.ext_clear_storage(key_data, key_length);
  }

  runtime::SizeType ExtensionImpl::ext_exists_storage(
      runtime::WasmPointer key_data, runtime::SizeType key_length) const {
    return storage_ext_.ext_exists_storage(key_data, key_length);
  }

  runtime::WasmPointer ExtensionImpl::ext_get_allocated_storage(
      runtime::WasmPointer key_data,
      runtime::SizeType key_length,
      runtime::WasmPointer written) {
    return storage_ext_.ext_get_allocated_storage(
        key_data, key_length, written);
  }

  runtime::SizeType ExtensionImpl::ext_get_storage_into(
      runtime::WasmPointer key_data,
      runtime::SizeType key_length,
      runtime::WasmPointer value_data,
      runtime::SizeType value_length,
      runtime::SizeType value_offset) {
    return storage_ext_.ext_get_storage_into(
        key_data, key_length, value_data, value_length, value_offset);
  }

  void ExtensionImpl::ext_set_storage(runtime::WasmPointer key_data,
                                      runtime::SizeType key_length,
                                      runtime::WasmPointer value_data,
                                      runtime::SizeType value_length) {
    return storage_ext_.ext_set_storage(
        key_data, key_length, value_data, value_length);
  }

  void ExtensionImpl::ext_blake2_256_enumerated_trie_root(
      runtime::WasmPointer values_data,
      runtime::WasmPointer lens_data,
      runtime::SizeType lens_length,
      runtime::WasmPointer result) {
    return storage_ext_.ext_blake2_256_enumerated_trie_root(
        values_data, lens_data, lens_length, result);
  }

  runtime::SizeType ExtensionImpl::ext_storage_changes_root(
      runtime::WasmPointer parent_hash_data, runtime::WasmPointer result) {
    return storage_ext_.ext_storage_changes_root(parent_hash_data, result);
  }

  void ExtensionImpl::ext_storage_root(runtime::WasmPointer result) const {
    return storage_ext_.ext_storage_root(result);
  }

  // -------------------------Memory extensions--------------------------

  runtime::WasmPointer ExtensionImpl::ext_malloc(runtime::SizeType size) {
    return memory_ext_.ext_malloc(size);
  }

  void ExtensionImpl::ext_free(runtime::WasmPointer ptr) {
    memory_ext_.ext_free(ptr);
  }

  /// I/O extensions
  void ExtensionImpl::ext_print_hex(runtime::WasmPointer data,
                                    runtime::SizeType length) {
    io_ext_.ext_print_hex(data, length);
  }

  void ExtensionImpl::ext_print_num(uint64_t value) {
    io_ext_.ext_print_num(value);
  }

  void ExtensionImpl::ext_print_utf8(runtime::WasmPointer utf8_data,
                                     runtime::SizeType utf8_length) {
    io_ext_.ext_print_utf8(utf8_data, utf8_length);
  }

  /// cryptographic extensions
  void ExtensionImpl::ext_blake2_128(runtime::WasmPointer data,
                                     runtime::SizeType len,
                                     runtime::WasmPointer out) {
    crypto_ext_.ext_blake2_128(data, len, out);
  }

  void ExtensionImpl::ext_blake2_256(runtime::WasmPointer data,
                                     runtime::SizeType len,
                                     runtime::WasmPointer out) {
    crypto_ext_.ext_blake2_256(data, len, out);
  }

  void ExtensionImpl::ext_keccak_256(runtime::WasmPointer data,
                                     runtime::SizeType len,
                                     runtime::WasmPointer out) {
    crypto_ext_.ext_keccak_256(data, len, out);
  }

  runtime::SizeType ExtensionImpl::ext_ed25519_verify(
      runtime::WasmPointer msg_data,
      runtime::SizeType msg_len,
      runtime::WasmPointer sig_data,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_.ext_ed25519_verify(
        msg_data, msg_len, sig_data, pubkey_data);
  }

  runtime::SizeType ExtensionImpl::ext_sr25519_verify(
      runtime::WasmPointer msg_data,
      runtime::SizeType msg_len,
      runtime::WasmPointer sig_data,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_.ext_sr25519_verify(
        msg_data, msg_len, sig_data, pubkey_data);
  }

  void ExtensionImpl::ext_twox_64(runtime::WasmPointer data,
                                  runtime::SizeType len,
                                  runtime::WasmPointer out) {
    crypto_ext_.ext_twox_64(data, len, out);
  }

  void ExtensionImpl::ext_twox_128(runtime::WasmPointer data,
                                   runtime::SizeType len,
                                   runtime::WasmPointer out) {
    crypto_ext_.ext_twox_128(data, len, out);
  }

  void ExtensionImpl::ext_twox_256(runtime::WasmPointer data,
                                   runtime::SizeType len,
                                   runtime::WasmPointer out) {
    crypto_ext_.ext_twox_256(data, len, out);
  }

  /// Crypto extensions v1

  runtime::PointerSize ExtensionImpl::ext_ed25519_public_keys_v1(
      runtime::SizeType key_type) {
    return crypto_ext_.ext_ed25519_public_keys_v1(key_type);
  }

  runtime::WasmPointer ExtensionImpl::ext_ed25519_generate_v1(
      runtime::SizeType key_type, runtime::PointerSize seed) {
    return crypto_ext_.ext_ed25519_generate_v1(key_type, seed);
  }

  runtime::PointerSize ExtensionImpl::ext_ed25519_sign_v1(
      runtime::SizeType key_type,
      runtime::WasmPointer key,
      runtime::PointerSize msg_data) {
    return crypto_ext_.ext_ed25519_sign_v1(key_type, key, msg_data);
  }

  runtime::SizeType ExtensionImpl::ext_ed25519_verify_v1(
      runtime::WasmPointer sig_data,
      runtime::PointerSize msg,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_.ext_ed25519_verify_v1(sig_data, msg, pubkey_data);
  }

  runtime::PointerSize ExtensionImpl::ext_sr25519_public_keys_v1(
      runtime::SizeType key_type) {
    return crypto_ext_.ext_sr25519_public_keys_v1(key_type);
  }

  runtime::WasmPointer ExtensionImpl::ext_sr25519_generate_v1(
      runtime::SizeType key_type, runtime::PointerSize seed) {
    return crypto_ext_.ext_sr25519_generate_v1(key_type, seed);
  }

  runtime::PointerSize ExtensionImpl::ext_sr25519_sign_v1(
      runtime::SizeType key_type,
      runtime::WasmPointer key,
      runtime::PointerSize msg_data) {
    return crypto_ext_.ext_sr25519_sign_v1(key_type, key, msg_data);
  }

  runtime::SizeType ExtensionImpl::ext_sr25519_verify_v1(
      runtime::WasmPointer sig_data,
      runtime::PointerSize msg,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_.ext_sr25519_verify_v1(sig_data, msg, pubkey_data);
  }

  /// misc extensions
  uint64_t ExtensionImpl::ext_chain_id() const {
    return misc_ext_.ext_chain_id();
  }

}  // namespace kagome::extensions
