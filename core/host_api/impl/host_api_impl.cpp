/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/host_api_impl.hpp"

#include "common/bytestr.hpp"
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "host_api/impl/offchain_extension.hpp"
#include "host_api/impl/storage_util.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "storage/predefined_keys.hpp"

#define FFI                                            \
  Ffi ffi {                                            \
    memory_provider_->getCurrentMemory().value().get() \
  }

namespace kagome::host_api {
  /**
   * Helps reading arguments from wasm and writing result to wasm.
   */
  struct Ffi {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    runtime::Memory &memory;

    /**
     * Read bytes argument.
     */
    auto bytes(runtime::WasmSpan arg) {
      return memory.view(arg).value();
    }
    /**
     * Read clear_prefix limit argument.
     */
    auto limit(runtime::WasmSpan arg) {
      return scale::decode<ClearPrefixLimit>(bytes(arg)).value();
    }
    /**
     * Read child trie argument.
     */
    auto child(runtime::WasmSpan arg) {
      return Buffer{storage::kChildStorageDefaultPrefix}.put(bytes(arg));
    }
    /**
     * Read `StateVersion` argument.
     */
    auto version(runtime::WasmI32 version) {
      return detail::toStateVersion(version);
    }
    /**
     * Write bytes result.
     */
    runtime::WasmSpan bytes(BufferView r) {
      return memory.storeBuffer(r);
    }
    /**
     * Write scale encoded result.
     */
    runtime::WasmSpan scale(const auto &r) {
      return bytes(scale::encode(r).value());
    }
  };

  HostApiImpl::HostApiImpl(
      const OffchainExtensionConfig &offchain_config,
      std::shared_ptr<const runtime::MemoryProvider> memory_provider,
      std::shared_ptr<const runtime::CoreApiFactory> core_provider,
      std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
      std::shared_ptr<const crypto::Sr25519Provider> sr25519_provider,
      std::shared_ptr<const crypto::EcdsaProvider> ecdsa_provider,
      std::shared_ptr<const crypto::Ed25519Provider> ed25519_provider,
      std::shared_ptr<const crypto::Secp256k1Provider> secp256k1_provider,
      std::shared_ptr<const crypto::EllipticCurves> elliptic_curves,
      std::shared_ptr<const crypto::Hasher> hasher,
      std::optional<std::shared_ptr<crypto::KeyStore>> key_store,
      std::shared_ptr<offchain::OffchainPersistentStorage>
          offchain_persistent_storage,
      std::shared_ptr<offchain::OffchainWorkerPool> offchain_worker_pool)
      : memory_provider_([&] {
          BOOST_ASSERT(memory_provider);
          return std::move(memory_provider);
        }()),
        storage_provider_([&] {
          BOOST_ASSERT(storage_provider);
          return std::move(storage_provider);
        }()),
        crypto_ext_(memory_provider_,
                    std::move(sr25519_provider),
                    std::move(ecdsa_provider),
                    std::move(ed25519_provider),
                    std::move(secp256k1_provider),
                    hasher,
                    std::move(key_store)),
        elliptic_curves_ext_(memory_provider_, std::move(elliptic_curves)),
        io_ext_(memory_provider_),
        memory_ext_(memory_provider_),
        misc_ext_{DEFAULT_CHAIN_ID,
                  hasher,
                  memory_provider_,
                  storage_provider_,
                  std::move(core_provider)},
        storage_ext_(storage_provider_, memory_provider_, hasher),
        child_storage_ext_(storage_provider_, memory_provider_),
        offchain_ext_(offchain_config,
                      memory_provider_,
                      std::move(offchain_persistent_storage),
                      std::move(offchain_worker_pool)) {}

  void HostApiImpl::reset() {
    storage_ext_.reset();
    crypto_ext_.reset();
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
    FFI;
    return storage_ext_.ext_storage_clear_prefix_version_1(ffi.bytes(prefix));
  }

  runtime::WasmSpan HostApiImpl::ext_storage_clear_prefix_version_2(
      runtime::WasmSpan prefix, runtime::WasmSpan limit) {
    FFI;
    return ffi.scale(storage_ext_.ext_storage_clear_prefix_version_2(
        ffi.bytes(prefix), ffi.limit(limit)));
  }

  runtime::WasmSpan HostApiImpl::ext_storage_root_version_1() {
    FFI;
    return ffi.bytes(storage_ext_.ext_storage_root_version_1());
  }

  runtime::WasmSpan HostApiImpl::ext_storage_root_version_2(
      runtime::WasmI32 state_version) {
    FFI;
    return ffi.bytes(
        storage_ext_.ext_storage_root_version_2(ffi.version(state_version)));
  }

  runtime::WasmSpan HostApiImpl::ext_storage_changes_root_version_1(
      runtime::WasmSpan parent_hash) {
    return storage_ext_.ext_storage_changes_root_version_1(parent_hash);
  }

  void HostApiImpl::ext_storage_start_transaction_version_1() {
    return storage_ext_.ext_storage_start_transaction_version_1();
  }

  void HostApiImpl::ext_storage_rollback_transaction_version_1() {
    return storage_ext_.ext_storage_rollback_transaction_version_1();
  }

  void HostApiImpl::ext_storage_commit_transaction_version_1() {
    return storage_ext_.ext_storage_commit_transaction_version_1();
  }

  runtime::WasmPointer HostApiImpl::ext_trie_blake2_256_root_version_1(
      runtime::WasmSpan values_data) {
    return storage_ext_.ext_trie_blake2_256_root_version_1(values_data);
  }

  runtime::WasmPointer HostApiImpl::ext_trie_blake2_256_ordered_root_version_1(
      runtime::WasmSpan values_data) {
    return storage_ext_.ext_trie_blake2_256_ordered_root_version_1(values_data);
  }

  runtime::WasmPointer HostApiImpl::ext_trie_blake2_256_ordered_root_version_2(
      runtime::WasmSpan values_data, runtime::WasmI32 state_version) {
    return storage_ext_.ext_trie_blake2_256_ordered_root_version_2(
        values_data, state_version);
  }

  runtime::WasmPointer HostApiImpl::ext_trie_keccak_256_ordered_root_version_2(
      runtime::WasmSpan values_data, runtime::WasmI32 state_version) {
    return storage_ext_.ext_trie_keccak_256_ordered_root_version_2(
        values_data, state_version);
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
    return crypto_ext_.ext_crypto_start_batch_verify_version_1();
  }

  int32_t HostApiImpl::ext_crypto_finish_batch_verify_version_1() {
    return crypto_ext_.ext_crypto_finish_batch_verify_version_1();
  }

  runtime::WasmSpan HostApiImpl::ext_crypto_ed25519_public_keys_version_1(
      runtime::WasmSize key_type) {
    return crypto_ext_.ext_crypto_ed25519_public_keys_version_1(key_type);
  }

  runtime::WasmPointer HostApiImpl::ext_crypto_ed25519_generate_version_1(
      runtime::WasmSize key_type, runtime::WasmSpan seed) {
    return crypto_ext_.ext_crypto_ed25519_generate_version_1(key_type, seed);
  }

  runtime::WasmSpan HostApiImpl::ext_crypto_ed25519_sign_version_1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg_data) {
    return crypto_ext_.ext_crypto_ed25519_sign_version_1(
        key_type, key, msg_data);
  }

  runtime::WasmSize HostApiImpl::ext_crypto_ed25519_verify_version_1(
      runtime::WasmPointer sig_data,
      runtime::WasmSpan msg,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_.ext_crypto_ed25519_verify_version_1(
        sig_data, msg, pubkey_data);
  }

  runtime::WasmSize HostApiImpl::ext_crypto_ed25519_batch_verify_version_1(
      runtime::WasmPointer sig_data,
      runtime::WasmSpan msg,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_.ext_crypto_ed25519_batch_verify_version_1(
        sig_data, msg, pubkey_data);
  }

  runtime::WasmSpan HostApiImpl::ext_crypto_sr25519_public_keys_version_1(
      runtime::WasmSize key_type) {
    return crypto_ext_.ext_crypto_sr25519_public_keys_version_1(key_type);
  }

  runtime::WasmPointer HostApiImpl::ext_crypto_sr25519_generate_version_1(
      runtime::WasmSize key_type, runtime::WasmSpan seed) {
    return crypto_ext_.ext_crypto_sr25519_generate_version_1(key_type, seed);
  }

  runtime::WasmSpan HostApiImpl::ext_crypto_sr25519_sign_version_1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg_data) {
    return crypto_ext_.ext_crypto_sr25519_sign_version_1(
        key_type, key, msg_data);
  }

  int32_t HostApiImpl::ext_crypto_sr25519_verify_version_1(
      runtime::WasmPointer sig_data,
      runtime::WasmSpan msg,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_.ext_crypto_sr25519_verify_version_1(
        sig_data, msg, pubkey_data);
  }

  int32_t HostApiImpl::ext_crypto_sr25519_verify_version_2(
      runtime::WasmPointer sig_data,
      runtime::WasmSpan msg,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_.ext_crypto_sr25519_verify_version_2(
        sig_data, msg, pubkey_data);
  }

  int32_t HostApiImpl::ext_crypto_sr25519_batch_verify_version_1(
      runtime::WasmPointer sig_data,
      runtime::WasmSpan msg,
      runtime::WasmPointer pubkey_data) {
    return crypto_ext_.ext_crypto_sr25519_batch_verify_version_1(
        sig_data, msg, pubkey_data);
  }

  runtime::WasmSpan HostApiImpl::ext_crypto_ecdsa_public_keys_version_1(
      runtime::WasmSize key_type) {
    return crypto_ext_.ext_crypto_ecdsa_public_keys_version_1(key_type);
  }

  runtime::WasmSpan HostApiImpl::ext_crypto_ecdsa_sign_version_1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg_data) {
    return crypto_ext_.ext_crypto_ecdsa_sign_version_1(key_type, key, msg_data);
  }

  runtime::WasmSpan HostApiImpl::ext_crypto_ecdsa_sign_prehashed_version_1(
      runtime::WasmSize key_type,
      runtime::WasmPointer key,
      runtime::WasmSpan msg_data) {
    return crypto_ext_.ext_crypto_ecdsa_sign_prehashed_version_1(
        key_type, key, msg_data);
  }

  runtime::WasmPointer HostApiImpl::ext_crypto_ecdsa_generate_version_1(
      runtime::WasmSize key_type_id, runtime::WasmSpan seed) {
    return crypto_ext_.ext_crypto_ecdsa_generate_version_1(key_type_id, seed);
  }

  int32_t HostApiImpl::ext_crypto_ecdsa_verify_version_1(
      runtime::WasmPointer sig,
      runtime::WasmSpan msg,
      runtime::WasmPointer key) {
    return crypto_ext_.ext_crypto_ecdsa_verify_version_1(sig, msg, key);
  }

  int32_t HostApiImpl::ext_crypto_ecdsa_verify_version_2(
      runtime::WasmPointer sig,
      runtime::WasmSpan msg,
      runtime::WasmPointer key) {
    return crypto_ext_.ext_crypto_ecdsa_verify_version_2(sig, msg, key);
  }

  int32_t HostApiImpl::ext_crypto_ecdsa_verify_prehashed_version_1(
      runtime::WasmPointer sig,
      runtime::WasmPointer msg,
      runtime::WasmPointer key) {
    return crypto_ext_.ext_crypto_ecdsa_verify_prehashed_version_1(
        sig, msg, key);
  }

  runtime::WasmPointer HostApiImpl::ext_crypto_bandersnatch_generate_version_1(
      runtime::WasmSize key_type, runtime::WasmSpan seed) {
    return crypto_ext_.ext_crypto_bandersnatch_generate_version_1(key_type,
                                                                  seed);
  }

  // ------------------------- Hashing extension/crypto ---------------

  runtime::WasmPointer HostApiImpl::ext_hashing_keccak_256_version_1(
      runtime::WasmSpan data) {
    return crypto_ext_.ext_hashing_keccak_256_version_1(data);
  }

  runtime::WasmPointer HostApiImpl::ext_hashing_sha2_256_version_1(
      runtime::WasmSpan data) {
    return crypto_ext_.ext_hashing_sha2_256_version_1(data);
  }

  runtime::WasmPointer HostApiImpl::ext_hashing_blake2_128_version_1(
      runtime::WasmSpan data) {
    return crypto_ext_.ext_hashing_blake2_128_version_1(data);
  }

  runtime::WasmPointer HostApiImpl::ext_hashing_blake2_256_version_1(
      runtime::WasmSpan data) {
    return crypto_ext_.ext_hashing_blake2_256_version_1(data);
  }

  runtime::WasmPointer HostApiImpl::ext_hashing_twox_64_version_1(
      runtime::WasmSpan data) {
    return crypto_ext_.ext_hashing_twox_64_version_1(data);
  }

  runtime::WasmPointer HostApiImpl::ext_hashing_twox_128_version_1(
      runtime::WasmSpan data) {
    return crypto_ext_.ext_hashing_twox_128_version_1(data);
  }

  runtime::WasmPointer HostApiImpl::ext_hashing_twox_256_version_1(
      runtime::WasmSpan data) {
    return crypto_ext_.ext_hashing_twox_256_version_1(data);
  }

  runtime::WasmSpan HostApiImpl::ext_misc_runtime_version_version_1(
      runtime::WasmSpan data) const {
    return misc_ext_.ext_misc_runtime_version_version_1(data);
  }

  void HostApiImpl::ext_misc_print_hex_version_1(runtime::WasmSpan data) const {
    return misc_ext_.ext_misc_print_hex_version_1(data);
  }

  void HostApiImpl::ext_misc_print_num_version_1(int64_t value) const {
    return misc_ext_.ext_misc_print_num_version_1(value);
  }

  void HostApiImpl::ext_misc_print_utf8_version_1(
      runtime::WasmSpan data) const {
    return misc_ext_.ext_misc_print_utf8_version_1(data);
  }

  runtime::WasmSpan HostApiImpl::ext_crypto_secp256k1_ecdsa_recover_version_1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    return crypto_ext_.ext_crypto_secp256k1_ecdsa_recover_version_1(sig, msg);
  }

  runtime::WasmSpan HostApiImpl::ext_crypto_secp256k1_ecdsa_recover_version_2(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    return crypto_ext_.ext_crypto_secp256k1_ecdsa_recover_version_2(sig, msg);
  }

  runtime::WasmSpan
  HostApiImpl::ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    return crypto_ext_.ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(
        sig, msg);
  }

  runtime::WasmSpan
  HostApiImpl::ext_crypto_secp256k1_ecdsa_recover_compressed_version_2(
      runtime::WasmPointer sig, runtime::WasmPointer msg) {
    return crypto_ext_.ext_crypto_secp256k1_ecdsa_recover_compressed_version_2(
        sig, msg);
  }

  // --------------------------- Offchain extension ----------------------------

  runtime::WasmI32 HostApiImpl::ext_offchain_is_validator_version_1() {
    return offchain_ext_.ext_offchain_is_validator_version_1();
  }

  runtime::WasmSpan HostApiImpl::ext_offchain_submit_transaction_version_1(
      runtime::WasmSpan data) {
    return offchain_ext_.ext_offchain_submit_transaction_version_1(data);
  }

  runtime::WasmSpan HostApiImpl::ext_offchain_network_state_version_1() {
    return offchain_ext_.ext_offchain_network_state_version_1();
  }

  runtime::WasmI64 HostApiImpl::ext_offchain_timestamp_version_1() {
    return offchain_ext_.ext_offchain_timestamp_version_1();
  }

  void HostApiImpl::ext_offchain_sleep_until_version_1(
      runtime::WasmI64 deadline) {
    return offchain_ext_.ext_offchain_sleep_until_version_1(deadline);
  }

  runtime::WasmPointer HostApiImpl::ext_offchain_random_seed_version_1() {
    return offchain_ext_.ext_offchain_random_seed_version_1();
  }

  void HostApiImpl::ext_offchain_local_storage_set_version_1(
      runtime::WasmI32 kind, runtime::WasmSpan key, runtime::WasmSpan value) {
    return offchain_ext_.ext_offchain_local_storage_set_version_1(
        kind, key, value);
  }

  void HostApiImpl::ext_offchain_local_storage_clear_version_1(
      runtime::WasmI32 kind, runtime::WasmSpan key) {
    return offchain_ext_.ext_offchain_local_storage_clear_version_1(kind, key);
  }

  runtime::WasmI32
  HostApiImpl::ext_offchain_local_storage_compare_and_set_version_1(
      runtime::WasmI32 kind,
      runtime::WasmSpan key,
      runtime::WasmSpan expected,
      runtime::WasmSpan value) {
    return offchain_ext_.ext_offchain_local_storage_compare_and_set_version_1(
        kind, key, expected, value);
  }

  runtime::WasmSpan HostApiImpl::ext_offchain_local_storage_get_version_1(
      runtime::WasmI32 kind, runtime::WasmSpan key) {
    return offchain_ext_.ext_offchain_local_storage_get_version_1(kind, key);
  }

  runtime::WasmSpan HostApiImpl::ext_offchain_http_request_start_version_1(
      runtime::WasmSpan method, runtime::WasmSpan uri, runtime::WasmSpan meta) {
    return offchain_ext_.ext_offchain_http_request_start_version_1(
        method, uri, meta);
  }

  runtime::WasmSpan HostApiImpl::ext_offchain_http_request_add_header_version_1(
      runtime::WasmI32 request_id,
      runtime::WasmSpan name,
      runtime::WasmSpan value) {
    return offchain_ext_.ext_offchain_http_request_add_header_version_1(
        request_id, name, value);
  }

  runtime::WasmSpan HostApiImpl::ext_offchain_http_request_write_body_version_1(
      runtime::WasmI32 request_id,
      runtime::WasmSpan chunk,
      runtime::WasmSpan deadline) {
    return offchain_ext_.ext_offchain_http_request_write_body_version_1(
        request_id, chunk, deadline);
  }

  runtime::WasmSpan HostApiImpl::ext_offchain_http_response_wait_version_1(
      runtime::WasmSpan ids, runtime::WasmSpan deadline) {
    return offchain_ext_.ext_offchain_http_response_wait_version_1(ids,
                                                                   deadline);
  }

  runtime::WasmSpan HostApiImpl::ext_offchain_http_response_headers_version_1(
      runtime::WasmI32 request_id) {
    return offchain_ext_.ext_offchain_http_response_headers_version_1(
        request_id);
  }

  runtime::WasmSpan HostApiImpl::ext_offchain_http_response_read_body_version_1(
      runtime::WasmI32 request_id,
      runtime::WasmSpan buffer,
      runtime::WasmSpan deadline) {
    return offchain_ext_.ext_offchain_http_response_read_body_version_1(
        request_id, buffer, deadline);
  }

  void HostApiImpl::ext_offchain_set_authorized_nodes_version_1(
      runtime::WasmSpan nodes, runtime::WasmI32 authorized_only) {
    return offchain_ext_.ext_offchain_set_authorized_nodes_version_1(
        nodes, authorized_only);
  }

  void HostApiImpl::ext_offchain_index_set_version_1(runtime::WasmSpan key,
                                                     runtime::WasmSpan value) {
    return offchain_ext_.ext_offchain_index_set_version_1(key, value);
  }

  void HostApiImpl::ext_offchain_index_clear_version_1(runtime::WasmSpan key) {
    return offchain_ext_.ext_offchain_index_clear_version_1(key);
  }

  void HostApiImpl::ext_default_child_storage_set_version_1(
      runtime::WasmSpan child_storage_key,
      runtime::WasmSpan key,
      runtime::WasmSpan value) {
    child_storage_ext_.ext_default_child_storage_set_version_1(
        child_storage_key, key, value);
  }

  runtime::WasmSpan HostApiImpl::ext_default_child_storage_get_version_1(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan key) const {
    return child_storage_ext_.ext_default_child_storage_get_version_1(
        child_storage_key, key);
  }

  void HostApiImpl::ext_default_child_storage_clear_version_1(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan key) {
    child_storage_ext_.ext_default_child_storage_clear_version_1(
        child_storage_key, key);
  }

  runtime::WasmSpan HostApiImpl::ext_default_child_storage_next_key_version_1(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan key) const {
    return child_storage_ext_.ext_default_child_storage_next_key_version_1(
        child_storage_key, key);
  }

  runtime::WasmSpan HostApiImpl::ext_default_child_storage_root_version_1(
      runtime::WasmSpan child_storage_key) const {
    FFI;
    return ffi.bytes(
        child_storage_ext_.ext_default_child_storage_root_version_1(
            ffi.child(child_storage_key)));
  }

  runtime::WasmSpan HostApiImpl::ext_default_child_storage_root_version_2(
      runtime::WasmSpan child_storage_key,
      runtime::WasmI32 state_version) const {
    FFI;
    return ffi.bytes(
        child_storage_ext_.ext_default_child_storage_root_version_2(
            ffi.child(child_storage_key), ffi.version(state_version)));
  }

  void HostApiImpl::ext_default_child_storage_clear_prefix_version_1(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan prefix) {
    FFI;
    return child_storage_ext_.ext_default_child_storage_clear_prefix_version_1(
        ffi.child(child_storage_key), ffi.bytes(prefix));
  }

  runtime::WasmSpan
  HostApiImpl::ext_default_child_storage_clear_prefix_version_2(
      runtime::WasmSpan child_storage_key,
      runtime::WasmSpan prefix,
      runtime::WasmSpan limit) {
    FFI;
    return ffi.scale(
        child_storage_ext_.ext_default_child_storage_clear_prefix_version_2(
            ffi.child(child_storage_key), ffi.bytes(prefix), ffi.limit(limit)));
  }

  runtime::WasmSpan HostApiImpl::ext_default_child_storage_read_version_1(
      runtime::WasmSpan child_storage_key,
      runtime::WasmSpan key,
      runtime::WasmSpan value_out,
      runtime::WasmOffset offset) const {
    return child_storage_ext_.ext_default_child_storage_read_version_1(
        child_storage_key, key, value_out, offset);
  }

  int32_t HostApiImpl::ext_default_child_storage_exists_version_1(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan key) const {
    return child_storage_ext_.ext_default_child_storage_exists_version_1(
        child_storage_key, key);
  }

  void HostApiImpl::ext_default_child_storage_storage_kill_version_1(
      runtime::WasmSpan child_storage_key) {
    FFI;
    return child_storage_ext_.ext_default_child_storage_storage_kill_version_1(
        ffi.child(child_storage_key));
  }

  runtime::WasmSpan
  HostApiImpl::ext_default_child_storage_storage_kill_version_3(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan limit) {
    FFI;
    return ffi.scale(
        child_storage_ext_.ext_default_child_storage_storage_kill_version_3(
            ffi.child(child_storage_key), ffi.limit(limit)));
  }

  void HostApiImpl::ext_panic_handler_abort_on_panic_version_1(
      runtime::WasmSpan message) {
    auto msg = byte2str(
        memory_provider_->getCurrentMemory()->get().view(message).value());
    throw std::runtime_error{std::string{msg}};
  }

  // ---------------------------- Elliptic Curves ----------------------------

  runtime::WasmSpan
  HostApiImpl::ext_elliptic_curves_bls12_381_multi_miller_loop_version_1(
      runtime::WasmSpan a, runtime::WasmSpan b) const {
    return elliptic_curves_ext_
        .ext_elliptic_curves_bls12_381_multi_miller_loop_version_1(a, b);
  }

  runtime::WasmSpan
  HostApiImpl::ext_elliptic_curves_bls12_381_final_exponentiation_version_1(
      runtime::WasmSpan f) const {
    return elliptic_curves_ext_
        .ext_elliptic_curves_bls12_381_final_exponentiation_version_1(f);
  }

  runtime::WasmSpan
  HostApiImpl::ext_elliptic_curves_bls12_381_mul_projective_g1_version_1(
      runtime::WasmSpan base, runtime::WasmSpan scalar) const {
    return elliptic_curves_ext_
        .ext_elliptic_curves_bls12_381_mul_projective_g1_version_1(base,
                                                                   scalar);
  }

  runtime::WasmSpan
  HostApiImpl::ext_elliptic_curves_bls12_381_mul_projective_g2_version_1(
      runtime::WasmSpan base, runtime::WasmSpan scalar) const {
    return elliptic_curves_ext_
        .ext_elliptic_curves_bls12_381_mul_projective_g2_version_1(base,
                                                                   scalar);
  }

  runtime::WasmSpan HostApiImpl::ext_elliptic_curves_bls12_381_msm_g1_version_1(
      runtime::WasmSpan bases, runtime::WasmSpan scalars) const {
    return elliptic_curves_ext_.ext_elliptic_curves_bls12_381_msm_g1_version_1(
        bases, scalars);
  }

  runtime::WasmSpan HostApiImpl::ext_elliptic_curves_bls12_381_msm_g2_version_1(
      runtime::WasmSpan bases, runtime::WasmSpan scalars) const {
    return elliptic_curves_ext_.ext_elliptic_curves_bls12_381_msm_g2_version_1(
        bases, scalars);
  }

  runtime::WasmSpan HostApiImpl::
      ext_elliptic_curves_ed_on_bls12_381_bandersnatch_sw_mul_projective_version_1(
          runtime::WasmSpan base, runtime::WasmSpan scalar) const {
    return elliptic_curves_ext_
        .ext_elliptic_curves_ed_on_bls12_381_bandersnatch_sw_mul_projective_version_1(
            base, scalar);
  }

}  // namespace kagome::host_api
