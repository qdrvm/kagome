/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/extension_impl.hpp"

namespace extensions {
  /// storage extensions
  uint8_t *ExtensionImpl::ext_child_storage_root(
      const uint8_t *storage_key_data,
      uint32_t storage_key_length,
      uint32_t *written) {
    return storage_ext_.ext_child_storage_root(
        storage_key_data, storage_key_length, written);
  }
  void ExtensionImpl::ext_clear_child_storage(const uint8_t *storage_key_data,
                                              uint32_t storage_key_length,
                                              const uint8_t *key_data,
                                              uint32_t key_length) {
    storage_ext_.ext_clear_child_storage(
        storage_key_data, storage_key_length, key_data, key_length);
  }
  void ExtensionImpl::ext_clear_prefix(const uint8_t *prefix_data,
                                       uint32_t prefix_length) {
    storage_ext_.ext_clear_prefix(prefix_data, prefix_length);
  }
  void ExtensionImpl::ext_clear_storage(const uint8_t *key_data,
                                        uint32_t key_length) {
    storage_ext_.ext_clear_storage(key_data, key_length);
  }
  uint32_t ExtensionImpl::ext_exists_child_storage(
      const uint8_t *storage_key_data,
      uint32_t storage_key_length,
      const uint8_t *key_data,
      uint32_t key_length) {
    return storage_ext_.ext_exists_child_storage(
        storage_key_data, storage_key_length, key_data, key_length);
  }
  uint8_t *ExtensionImpl::ext_get_allocated_child_storage(
      const uint8_t *storage_key_data,
      uint32_t storage_key_length,
      const uint8_t *key_data,
      uint32_t key_length,
      uint32_t *written) {
    return storage_ext_.ext_get_allocated_child_storage(
        storage_key_data, storage_key_length, key_data, key_length, written);
  }
  uint8_t *ExtensionImpl::ext_get_allocated_storage(const uint8_t *key_data,
                                                    uint32_t key_length,
                                                    uint32_t *written) {
    return storage_ext_.ext_get_allocated_storage(
        key_data, key_length, written);
  }
  uint32_t ExtensionImpl::ext_get_child_storage_into(
      const uint8_t *storage_key_data,
      uint32_t storage_key_length,
      const uint8_t *key_data,
      uint32_t key_length,
      uint8_t *value_data,
      uint32_t value_length,
      uint32_t value_offset) {
    return storage_ext_.ext_get_child_storage_into(storage_key_data,
                                                   storage_key_length,
                                                   key_data,
                                                   key_length,
                                                   value_data,
                                                   value_length,
                                                   value_offset);
  }
  uint32_t ExtensionImpl::ext_get_storage_into(const uint8_t *key_data,
                                               uint32_t key_length,
                                               uint8_t *value_data,
                                               uint32_t value_length,
                                               uint32_t value_offset) {
    return storage_ext_.ext_get_storage_into(
        key_data, key_length, value_data, value_length, value_offset);
  }
  void ExtensionImpl::ext_kill_child_storage(const uint8_t *storage_key_data,
                                             uint32_t storage_key_length) {
    storage_ext_.ext_kill_child_storage(storage_key_data, storage_key_length);
  }
  void ExtensionImpl::ext_set_child_storage(const uint8_t *storage_key_data,
                                            uint32_t storage_key_length,
                                            const uint8_t *key_data,
                                            uint32_t key_length,
                                            const uint8_t *value_data,
                                            uint32_t value_length) {
    storage_ext_.ext_set_child_storage(storage_key_data,
                                       storage_key_length,
                                       key_data,
                                       key_length,
                                       value_data,
                                       value_length);
  }
  void ExtensionImpl::ext_set_storage(const uint8_t *key_data,
                                      uint32_t key_length,
                                      const uint8_t *value_data,
                                      uint32_t value_length) {
    storage_ext_.ext_set_storage(
        key_data, key_length, value_data, value_length);
  }
  uint32_t ExtensionImpl::ext_storage_changes_root(
      const uint8_t *parent_hash_data,
      uint32_t parent_hash_len,
      uint64_t parent_num,
      uint8_t *result) {
    return storage_ext_.ext_storage_changes_root(
        parent_hash_data, parent_hash_len, parent_num, result);
  }
  void ExtensionImpl::ext_storage_root(uint8_t *result) {
    storage_ext_.ext_storage_root(result);
  }
  uint32_t ExtensionImpl::ext_exists_storage(const uint8_t *key_data,
                                             uint32_t key_length) {
    return storage_ext_.ext_exists_storage(key_data, key_length);
  }

  /// memory extensions
  uint8_t *ExtensionImpl::ext_malloc(uint32_t size) {
    return memory_ext_.ext_malloc(size);
  }
  void ExtensionImpl::ext_free(uint8_t *ptr) {
    memory_ext_.ext_free(ptr);
  }

  /// I/O extensions
  void ExtensionImpl::ext_print_hex(const uint8_t *data, uint32_t length) {
    io_ext_.ext_print_hex(data, length);
  }
  void ExtensionImpl::ext_print_num(uint64_t value) {
    io_ext_.ext_print_num(value);
  }
  void ExtensionImpl::ext_print_utf8(const uint8_t *utf8_data,
                                     uint32_t utf8_length) {
    io_ext_.ext_print_utf8(utf8_data, utf8_length);
  }

  /// cryptographic extensions
  void ExtensionImpl::ext_blake2_256(const uint8_t *data,
                                     uint32_t len,
                                     uint8_t *out) {
    crypto_ext_.ext_blake2_256(data, len, out);
  }
  void ExtensionImpl::ext_blake2_256_enumerated_trie_root(
      const uint8_t *values_data,
      const uint32_t *lens_data,
      uint32_t lens_length,
      uint8_t *result) {
    crypto_ext_.ext_blake2_256_enumerated_trie_root(
        values_data, lens_data, lens_length, result);
  }
  uint32_t ExtensionImpl::ext_ed25519_verify(const uint8_t *msg_data,
                                             uint32_t msg_len,
                                             const uint8_t *sig_data,
                                             const uint8_t *pubkey_data) {
    return crypto_ext_.ext_ed25519_verify(
        msg_data, msg_len, sig_data, pubkey_data);
  }
  void ExtensionImpl::ext_twox_128(const uint8_t *data,
                                   uint32_t len,
                                   uint8_t *out) {
    crypto_ext_.ext_twox_128(data, len, out);
  }
  void ExtensionImpl::ext_twox_256(const uint8_t *data,
                                   uint32_t len,
                                   uint8_t *out) {
    crypto_ext_.ext_twox_256(data, len, out);
  }

  /// sandboxing extensions
  void ExtensionImpl::ext_sandbox_instance_teardown(uint32_t instance_idx) {
    sandboxing_ext_.ext_sandbox_instance_teardown(instance_idx);
  }
  uint32_t ExtensionImpl::ext_sandbox_instantiate(
      std::function<uint64_t(const uint8_t *serialized_args,
                             size_t serialized_args_length,
                             size_t state,
                             size_t func_index)> dispatch_func,
      const uint8_t *wasm_ptr,
      size_t wasm_length,
      const uint8_t *imports_ptr,
      size_t imports_length,
      size_t state) {
    return sandboxing_ext_.ext_sandbox_instantiate(dispatch_func,
                                                   wasm_ptr,
                                                   wasm_length,
                                                   imports_ptr,
                                                   imports_length,
                                                   state);
  }
  uint32_t ExtensionImpl::ext_sandbox_invoke(uint32_t instance_idx,
                                             const uint8_t *export_ptr,
                                             size_t export_len,
                                             const uint8_t *args_ptr,
                                             size_t args_len,
                                             uint8_t *return_val_ptr,
                                             size_t return_val_len,
                                             size_t state) {
    return sandboxing_ext_.ext_sandbox_invoke(instance_idx,
                                              export_ptr,
                                              export_len,
                                              args_ptr,
                                              args_len,
                                              return_val_ptr,
                                              return_val_len,
                                              state);
  }
  uint32_t ExtensionImpl::ext_sandbox_memory_get(uint32_t memory_idx,
                                                 uint32_t offset,
                                                 uint8_t *buf_ptr,
                                                 size_t buf_length) {
    return sandboxing_ext_.ext_sandbox_memory_get(
        memory_idx, offset, buf_ptr, buf_length);
  }
  uint32_t ExtensionImpl::ext_sandbox_memory_new(uint32_t initial,
                                                 uint32_t maximum) {
    return sandboxing_ext_.ext_sandbox_memory_new(initial, maximum);
  }
  uint32_t ExtensionImpl::ext_sandbox_memory_set(uint32_t memory_idx,
                                                 uint32_t offset,
                                                 const uint8_t *val_ptr,
                                                 size_t val_len) {
    return sandboxing_ext_.ext_sandbox_memory_set(
        memory_idx, offset, val_ptr, val_len);
  }
  void ExtensionImpl::ext_sandbox_memory_teardown(uint32_t memory_idx) {
    sandboxing_ext_.ext_sandbox_memory_teardown(memory_idx);
  }

  /// misc extensions
  uint64_t ExtensionImpl::ext_chain_id() {
    return misc_ext_.ext_chain_id();
  }
}  // namespace extensions
