/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EXTENSION_IMPL_HPP
#define KAGOME_EXTENSION_IMPL_HPP

#include "extensions/extension.hpp"
#include "extensions/impl/crypto_extension.hpp"
#include "extensions/impl/io_extension.hpp"
#include "extensions/impl/memory_extension.hpp"
#include "extensions/impl/misc_extension.hpp"
#include "extensions/impl/sandboxing_extension.hpp"
#include "extensions/impl/storage_extension.hpp"

namespace extensions {
  /**
   * Fair implementation of the extensions interface
   */
  class ExtensionImpl : public Extension {
   public:
    /// storage extensions
    uint8_t *ext_child_storage_root(const uint8_t *storage_key_data,
                                    uint32_t storage_key_length,
                                    uint32_t *written) override;
    void ext_clear_child_storage(const uint8_t *storage_key_data,
                                 uint32_t storage_key_length,
                                 const uint8_t *key_data,
                                 uint32_t key_length) override;
    void ext_clear_prefix(const uint8_t *prefix_data,
                          uint32_t prefix_length) override;
    void ext_clear_storage(const uint8_t *key_data,
                           uint32_t key_length) override;
    uint32_t ext_exists_child_storage(const uint8_t *storage_key_data,
                                      uint32_t storage_key_length,
                                      const uint8_t *key_data,
                                      uint32_t key_length) override;
    uint8_t *ext_get_allocated_child_storage(const uint8_t *storage_key_data,
                                             uint32_t storage_key_length,
                                             const uint8_t *key_data,
                                             uint32_t key_length,
                                             uint32_t *written) override;
    uint8_t *ext_get_allocated_storage(const uint8_t *key_data,
                                       uint32_t key_length,
                                       uint32_t *written) override;
    uint32_t ext_get_child_storage_into(const uint8_t *storage_key_data,
                                        uint32_t storage_key_length,
                                        const uint8_t *key_data,
                                        uint32_t key_length,
                                        uint8_t *value_data,
                                        uint32_t value_length,
                                        uint32_t value_offset) override;
    uint32_t ext_get_storage_into(const uint8_t *key_data,
                                  uint32_t key_length,
                                  uint8_t *value_data,
                                  uint32_t value_length,
                                  uint32_t value_offset) override;
    void ext_kill_child_storage(const uint8_t *storage_key_data,
                                uint32_t storage_key_length) override;
    void ext_set_child_storage(const uint8_t *storage_key_data,
                               uint32_t storage_key_length,
                               const uint8_t *key_data,
                               uint32_t key_length,
                               uint8_t *value_data,
                               uint32_t value_length) override;
    void ext_set_storage(const uint8_t *key_data,
                         uint32_t key_length,
                         uint8_t *value_data,
                         uint32_t value_length) override;
    uint32_t ext_storage_changes_root(const uint8_t *parent_hash_data,
                                      uint32_t parent_hash_len,
                                      uint64_t parent_num,
                                      uint8_t *result) override;
    void ext_storage_root(uint8_t *result) override;
    uint32_t ext_exists_storage(const uint8_t *key_data,
                                uint32_t key_length) override;

    /// memory extensions
    uint8_t *ext_malloc(uint32_t size) override;
    void ext_free(uint8_t *ptr) override;

    /// I/O extensions
    void ext_print_hex(const uint8_t *data, uint32_t length) override;
    void ext_print_num(uint64_t value) override;
    void ext_print_utf8(const uint8_t *utf8_data,
                        uint32_t utf8_length) override;

    /// cryptographic extensions
    void ext_blake2_256(const uint8_t *data,
                        uint32_t len,
                        uint8_t *out) override;
    void ext_blake2_256_enumerated_trie_root(const uint8_t *values_data,
                                             const uint32_t *lens_data,
                                             uint32_t lens_length,
                                             uint8_t *result) override;
    uint32_t ext_ed25519_verify(const uint8_t *msg_data,
                                uint32_t msg_len,
                                const uint8_t *sig_data,
                                const uint8_t *pubkey_data) override;
    void ext_twox_128(const uint8_t *data, uint32_t len, uint8_t *out) override;
    void ext_twox_256(const uint8_t *data, uint32_t len, uint8_t *out) override;

    /// sandboxing extensions
    void ext_sandbox_instance_teardown(uint32_t instance_idx) override;
    uint32_t ext_sandbox_instantiate(
        std::function<uint64_t(const uint8_t *serialized_args,
                               uint32_t serialized_args_length,
                               uint32_t state,
                               uint32_t func_index)> dispatch_func,
        const uint8_t *wasm_ptr,
        uint32_t wasm_length,
        const uint8_t *imports_ptr,
        uint32_t imports_length,
        uint32_t state) override;
    uint32_t ext_sandbox_invoke(uint32_t instance_idx,
                                const uint8_t *export_ptr,
                                uint32_t export_len,
                                const uint8_t *args_ptr,
                                uint32_t args_len,
                                uint8_t *return_val_ptr,
                                uint32_t return_val_len,
                                uint32_t state) override;
    uint32_t ext_sandbox_memory_get(uint32_t memory_idx,
                                    uint32_t offset,
                                    uint8_t *buf_ptr,
                                    uint32_t buf_length) override;
    uint32_t ext_sandbox_memory_new(uint32_t initial,
                                    uint32_t maximum) override;
    uint32_t ext_sandbox_memory_set(uint32_t memory_idx,
                                    uint32_t offset,
                                    const uint8_t *val_ptr,
                                    uint32_t val_len) override;
    void ext_sandbox_memory_teardown(uint32_t memory_idx) override;

    /// misc extensions
    uint64_t ext_chain_id() override;

   private:
    CryptoExtension crypto_ext_;
    IOExtension io_ext_;
    MemoryExtension memory_ext_;
    MiscExtension misc_ext_;
    SandboxingExtension sandboxing_ext_;
    StorageExtension storage_ext_;
  };
}  // namespace extensions

#endif  // KAGOME_EXTENSION_IMPL_HPP
