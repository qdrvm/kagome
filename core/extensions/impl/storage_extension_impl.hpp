/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EXTENSIONS_STORAGE_EXTENSION_IMPL_HPP
#define KAGOME_EXTENSIONS_STORAGE_EXTENSION_IMPL_HPP

#include <cstdint>

#include "common/logger.hpp"
#include "extensions/impl/storage_extension.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "runtime/wasm_memory.hpp"
#include "storage/changes_trie/changes_tracker.hpp"

namespace kagome::extensions {
  /**
   * Implements extension functions related to storage
   */
  class StorageExtensionImpl : public StorageExtension {
   public:
    StorageExtensionImpl(
        std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
        std::shared_ptr<runtime::WasmMemory> memory,
        std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker);

    // -------------------------Data storage--------------------------

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

    // -------------------------Trie operations--------------------------

    void ext_blake2_256_enumerated_trie_root(
        runtime::WasmPointer values_data,
        runtime::WasmPointer lengths_data,
        runtime::WasmSize values_num,
        runtime::WasmPointer result) override;

    runtime::WasmPointer ext_storage_changes_root(
        runtime::WasmPointer parent_hash, runtime::WasmPointer result) override;

    void ext_storage_root(runtime::WasmPointer result) const override;

    // ------------------------ VERSION 1 ------------------------

    void ext_storage_set_version_1(runtime::WasmSpan key,
                                   runtime::WasmSpan value) override;

    runtime::WasmSpan ext_storage_get_version_1(runtime::WasmSpan key) override;

    void ext_storage_clear_version_1(runtime::WasmSpan key_data) override;

    runtime::WasmSize ext_storage_exists_version_1(
        runtime::WasmSpan key_data) const override;

    void ext_storage_clear_prefix_version_1(runtime::WasmSpan prefix) override;

    runtime::WasmPointer ext_storage_root_version_1() const override;

    runtime::WasmPointer ext_storage_changes_root_version_1(
        runtime::WasmSpan parent_hash) override;

    runtime::WasmSpan ext_storage_next_key_version_1(
        runtime::WasmSpan key) const override;

    runtime::WasmPointer ext_trie_blake2_256_root_version_1(
        runtime::WasmSpan values_data) override;

    runtime::WasmPointer ext_trie_blake2_256_ordered_root_version_1(
        runtime::WasmSpan values_data) override;

   private:
    /**
     * Find the value by given key and the return the part of it starting from
     * given offset
     *
     * @param key Buffer representation of the key
     * @param offset SizeType pointing to the beginning of the value
     * @param max_length SizeType defining the maximum possible length of the
     * returned result
     * @return result containing Buffer with the part of the value, or error in
     * case value by give key does not exist
     */
    outcome::result<common::Buffer> get(const common::Buffer &key,
                                        runtime::WasmSize offset,
                                        runtime::WasmSize max_length) const;

    outcome::result<common::Buffer> get(const common::Buffer &key) const;

    outcome::result<boost::optional<common::Buffer>> getStorageNextKey(
        const common::Buffer &key) const;

    std::shared_ptr<runtime::TrieStorageProvider> storage_provider_;
    std::shared_ptr<runtime::WasmMemory> memory_;
    std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker_;
    common::Logger logger_;

    constexpr static auto kDefaultLoggerTag = "WASM Runtime [StorageExtension]";
  };
}  // namespace kagome::extensions

#endif  // KAGOME_EXTENSIONS_STORAGE_EXTENSION_IMPL_HPP
