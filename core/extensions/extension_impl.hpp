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

namespace kagome::extensions {
  /**
   * Fair implementation of the extensions interface
   */
  class ExtensionImpl : public Extension {
   public:
    ~ExtensionImpl() override  = default;

    // -------------------------Storage extensions--------------------------

    void ext_clear_prefix(runtime::WasmPointer prefix_data,
                          runtime::SizeType prefix_length) override;

    void ext_clear_storage(runtime::WasmPointer key_data,
                           runtime::SizeType key_length) override;

    runtime::SizeType ext_exists_storage(
        runtime::WasmPointer key_data,
        runtime::SizeType key_length) const override;

    runtime::WasmPointer ext_get_allocated_storage(
        runtime::WasmPointer key_data, runtime::SizeType key_length,
        runtime::WasmPointer len_ptr) override;

    runtime::SizeType ext_get_storage_into(
        runtime::WasmPointer key_data, runtime::SizeType key_length,
        runtime::WasmPointer value_data, runtime::SizeType value_length,
        runtime::SizeType value_offset) override;

    void ext_set_storage(runtime::WasmPointer key_data,
                         runtime::SizeType key_length,
                         runtime::WasmPointer value_data,
                         runtime::SizeType value_length) override;

    void ext_blake2_256_enumerated_trie_root(
        runtime::WasmPointer values_data, runtime::WasmPointer lens_data,
        runtime::SizeType lens_length, runtime::WasmPointer result) override;

    runtime::SizeType ext_storage_changes_root(
        runtime::WasmPointer parent_hash_data,
        runtime::SizeType parent_hash_len, runtime::SizeType parent_num,
        runtime::WasmPointer result) override;

    void ext_storage_root(runtime::WasmPointer result) const override;

    runtime::WasmPointer ext_child_storage_root(
        runtime::WasmPointer storage_key_data,
        runtime::WasmPointer storage_key_length,
        runtime::WasmPointer written) override;

    void ext_clear_child_storage(runtime::WasmPointer storage_key_data,
                                 runtime::WasmPointer storage_key_length,
                                 runtime::WasmPointer key_data,
                                 runtime::WasmPointer key_length) override;

    runtime::SizeType ext_exists_child_storage(
        runtime::WasmPointer storage_key_data,
        runtime::WasmPointer storage_key_length, runtime::WasmPointer key_data,
        runtime::WasmPointer key_length) override;

    runtime::WasmPointer ext_get_allocated_child_storage(
        runtime::WasmPointer storage_key_data,
        runtime::WasmPointer storage_key_length, runtime::WasmPointer key_data,
        runtime::WasmPointer key_length, runtime::WasmPointer written) override;

    runtime::SizeType ext_get_child_storage_into(
        runtime::WasmPointer storage_key_data,
        runtime::WasmPointer storage_key_length, runtime::WasmPointer key_data,
        runtime::WasmPointer key_length, runtime::WasmPointer value_data,
        runtime::SizeType value_length,
        runtime::SizeType value_offset) override;

    void ext_kill_child_storage(runtime::WasmPointer storage_key_data,
                                runtime::SizeType storage_key_length) override;

    void ext_set_child_storage(runtime::WasmPointer storage_key_data,
                               runtime::SizeType storage_key_length,
                               runtime::WasmPointer key_data,
                               runtime::SizeType key_length,
                               runtime::WasmPointer value_data,
                               runtime::SizeType value_length) override;

    // -------------------------Memory extensions--------------------------
    runtime::WasmPointer ext_malloc(runtime::SizeType size) override;

    void ext_free(runtime::WasmPointer ptr) override;

    // -------------------------I/O extensions--------------------------
    void ext_print_hex(const uint8_t *data, uint32_t length) override;

    void ext_print_num(uint64_t value) override;

    void ext_print_utf8(const uint8_t *utf8_data,
                        uint32_t utf8_length) override;

    // -------------------------Cryptographic extensions----------------------
    void ext_blake2_256(const uint8_t *data, uint32_t len,
                        uint8_t *out) override;

    uint32_t ext_ed25519_verify(const uint8_t *msg_data, uint32_t msg_len,
                                const uint8_t *sig_data,
                                const uint8_t *pubkey_data) override;

    void ext_twox_128(const uint8_t *data, uint32_t len, uint8_t *out) override;

    void ext_twox_256(const uint8_t *data, uint32_t len, uint8_t *out) override;

    // -------------------------Sandboxing extensions----------------------
    void ext_sandbox_instance_teardown(uint32_t instance_idx) override;

    uint32_t ext_sandbox_instantiate(
        const SandoxDispatchFuncType &dispatch_func, const uint8_t *wasm_ptr,
        size_t wasm_length, const uint8_t *imports_ptr, size_t imports_length,
        size_t state) override;

    uint32_t ext_sandbox_invoke(uint32_t instance_idx,
                                const uint8_t *export_ptr, size_t export_len,
                                const uint8_t *args_ptr, size_t args_len,
                                uint8_t *return_val_ptr, size_t return_val_len,
                                size_t state) override;

    uint32_t ext_sandbox_memory_get(uint32_t memory_idx, uint32_t offset,
                                    uint8_t *buf_ptr,
                                    size_t buf_length) override;

    uint32_t ext_sandbox_memory_new(uint32_t initial,
                                    uint32_t maximum) override;

    uint32_t ext_sandbox_memory_set(uint32_t memory_idx, uint32_t offset,
                                    const uint8_t *val_ptr,
                                    size_t val_len) override;

    void ext_sandbox_memory_teardown(uint32_t memory_idx) override;

    // -------------------------Misc extensions--------------------------

    uint64_t ext_chain_id() const override;

   private:
    CryptoExtension crypto_ext_;
    IOExtension io_ext_;
    MemoryExtension memory_ext_;
    MiscExtension misc_ext_;
    SandboxingExtension sandboxing_ext_;
    StorageExtension storage_ext_;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_EXTENSION_IMPL_HPP
