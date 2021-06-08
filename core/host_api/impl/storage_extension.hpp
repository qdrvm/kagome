/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EXTENSIONS_STORAGE_EXTENSION_HPP
#define KAGOME_EXTENSIONS_STORAGE_EXTENSION_HPP

#include <cstdint>

#include "log/logger.hpp"
#include "runtime/types.hpp"
#include "storage/changes_trie/changes_tracker.hpp"

namespace kagome::runtime {
  class MemoryProvider;
  class TrieStorageProvider;
}

namespace kagome::host_api {
  /**
   * Implements extension functions related to storage
   */
  class StorageExtension {
   public:
    StorageExtension(
        std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
        std::shared_ptr<const runtime::MemoryProvider> memory_provider,
        std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker);

    void reset();

    // -------------------------Trie operations--------------------------

    /**
     * @see Extension::ext_storage_read_version_1
     */
    runtime::WasmSpan ext_storage_read_version_1(runtime::WasmSpan key,
                                                 runtime::WasmSpan value_out,
                                                 runtime::WasmOffset offset);

    // ------------------------ VERSION 1 ------------------------

    /**
     * @see Extension::ext_storage_set_version_1
     */
    void ext_storage_set_version_1(runtime::WasmSpan key,
                                   runtime::WasmSpan value);

    /**
     * @see Extension::ext_storage_get_version_1
     */
    runtime::WasmSpan ext_storage_get_version_1(runtime::WasmSpan key);

    /**
     * @see Extension::ext_storage_clear_version_1
     */
    void ext_storage_clear_version_1(runtime::WasmSpan key_data);

    /**
     * @see Extension::ext_storage_exists_version_1
     */
    runtime::WasmSize ext_storage_exists_version_1(
        runtime::WasmSpan key_data) const;

    /**
     * @see Extension::ext_storage_clear_prefix_version_1
     */
    void ext_storage_clear_prefix_version_1(runtime::WasmSpan prefix);

    /**
     * @see Extension::ext_storage_root_version_1
     */
    runtime::WasmSpan ext_storage_root_version_1() const;

    /**
     * @see Extension::ext_storage_changes_root_version_1
     */
    runtime::WasmSpan ext_storage_changes_root_version_1(
        runtime::WasmSpan parent_hash);

    /**
     * @see Extension::ext_storage_next_key
     */
    runtime::WasmSpan ext_storage_next_key_version_1(
        runtime::WasmSpan key) const;

    /**
     * @see Extension::ext_storage_append_version_1
     */
    void ext_storage_append_version_1(runtime::WasmSpan key,
                                      runtime::WasmSpan value) const;

    /**
     * @see Extension::ext_trie_blake2_256_root_version_1
     */
    runtime::WasmPointer ext_trie_blake2_256_root_version_1(
        runtime::WasmSpan values_data);

    /**
     * @see Extension::ext_trie_blake2_256_ordered_root_version_1
     */
    runtime::WasmPointer ext_trie_blake2_256_ordered_root_version_1(
        runtime::WasmSpan values_data);

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

    /**
     * Find the value by given key and the return the part of it starting from
     * given offset
     *
     * @param key Buffer representation of the key
     * @return result containing Buffer with the value
     */
    outcome::result<common::Buffer> get(const common::Buffer &key) const;

    /**
     * @return error if any, a key if the next key exists
     * none otherwise
     */
    outcome::result<boost::optional<common::Buffer>> getStorageNextKey(
        const common::Buffer &key) const;

    boost::optional<common::Buffer> calcStorageChangesRoot(
        common::Hash256 parent) const;

    std::shared_ptr<runtime::TrieStorageProvider> storage_provider_;
    std::shared_ptr<const runtime::MemoryProvider> memory_provider_;
    std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker_;
    log::Logger logger_;

    constexpr static auto kDefaultLoggerTag = "WASM Runtime [StorageExtension]";
  };

}  // namespace kagome::host_api

#endif  // KAGOME_STORAGE_EXTENSIONS_EXTENSION_HPP
