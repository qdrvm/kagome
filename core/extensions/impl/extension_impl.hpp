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
#include "extensions/impl/storage_extension.hpp"
#include "storage/trie_db_overlay/trie_db_overlay.hpp"

namespace kagome::extensions {
  /**
   * Fair implementation of the extensions interface
   */
  class ExtensionImpl : public Extension {
   public:
    ExtensionImpl() = delete;
    ExtensionImpl(const std::shared_ptr<runtime::WasmMemory> &memory,
                  std::shared_ptr<storage::trie_db_overlay::TrieDbOverlay> db);

    ~ExtensionImpl() override = default;

    std::shared_ptr<runtime::WasmMemory> memory() const override;

    // -------------------------Storage extensions--------------------------

    void ext_clear_prefix(runtime::WasmPointer prefix_data,
                          runtime::SizeType prefix_length) override;

    void ext_clear_storage(runtime::WasmPointer key_data,
                           runtime::SizeType key_length) override;

    runtime::SizeType ext_exists_storage(
        runtime::WasmPointer key_data,
        runtime::SizeType key_length) const override;

    runtime::WasmPointer ext_get_allocated_storage(
        runtime::WasmPointer key_data,
        runtime::SizeType key_length,
        runtime::WasmPointer len_ptr) override;

    runtime::SizeType ext_get_storage_into(
        runtime::WasmPointer key_data,
        runtime::SizeType key_length,
        runtime::WasmPointer value_data,
        runtime::SizeType value_length,
        runtime::SizeType value_offset) override;

    void ext_set_storage(runtime::WasmPointer key_data,
                         runtime::SizeType key_length,
                         runtime::WasmPointer value_data,
                         runtime::SizeType value_length) override;

    void ext_blake2_256_enumerated_trie_root(
        runtime::WasmPointer values_data,
        runtime::WasmPointer lens_data,
        runtime::SizeType lens_length,
        runtime::WasmPointer result) override;

    runtime::SizeType ext_storage_changes_root(
        runtime::WasmPointer parent_hash_data,
        runtime::SizeType parent_hash_len,
        runtime::WasmPointer result) override;

    void ext_storage_root(runtime::WasmPointer result) const override;

    // -------------------------Memory extensions--------------------------
    runtime::WasmPointer ext_malloc(runtime::SizeType size) override;

    void ext_free(runtime::WasmPointer ptr) override;

    // -------------------------I/O extensions--------------------------
    void ext_print_hex(runtime::WasmPointer data,
                       runtime::SizeType length) override;

    void ext_print_num(uint64_t value) override;

    void ext_print_utf8(runtime::WasmPointer utf8_data,
                        runtime::SizeType utf8_length) override;

    // -------------------------Cryptographic extensions----------------------
    void ext_blake2_256(runtime::WasmPointer data,
                        runtime::SizeType len,
                        runtime::WasmPointer out) override;

    void ext_keccak_256(runtime::WasmPointer data,
                        runtime::SizeType len,
                        runtime::WasmPointer out) override;

    runtime::SizeType ext_ed25519_verify(
        runtime::WasmPointer msg_data,
        runtime::SizeType msg_len,
        runtime::WasmPointer sig_data,
        runtime::WasmPointer pubkey_data) override;

    runtime::SizeType ext_sr25519_verify(
        runtime::WasmPointer msg_data,
        runtime::SizeType msg_len,
        runtime::WasmPointer sig_data,
        runtime::WasmPointer pubkey_data) override;

    void ext_twox_64(runtime::WasmPointer data,
                      runtime::SizeType len,
                      runtime::WasmPointer out) override;

    void ext_twox_128(runtime::WasmPointer data,
                      runtime::SizeType len,
                      runtime::WasmPointer out) override;

    void ext_twox_256(runtime::WasmPointer data,
                      runtime::SizeType len,
                      runtime::WasmPointer out) override;
    // -------------------------Misc extensions--------------------------

    uint64_t ext_chain_id() const override;

   private:
    std::shared_ptr<runtime::WasmMemory> memory_;
    std::shared_ptr<storage::trie_db_overlay::TrieDbOverlay> db_;

    CryptoExtension crypto_ext_;
    IOExtension io_ext_;
    MemoryExtension memory_ext_;
    MiscExtension misc_ext_;
    StorageExtension storage_ext_;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_EXTENSION_IMPL_HPP
