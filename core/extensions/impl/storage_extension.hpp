/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EXTENSIONS_STORAGE_EXTENSION_HPP
#define KAGOME_EXTENSIONS_STORAGE_EXTENSION_HPP

#include <cstdint>

#include "common/logger.hpp"
#include "runtime/wasm_memory.hpp"
#include "storage/changes_trie/changes_tracker.hpp"
#include "storage/trie/trie_batches.hpp"

namespace kagome::extensions {
  /**
   * Implements extension functions related to storage
   */
  class StorageExtension {
   public:
    StorageExtension(
        std::shared_ptr<storage::trie::TrieBatch> storage_batch,
        std::shared_ptr<runtime::WasmMemory> memory,
        std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker);

    // -------------------------Data storage--------------------------

    /**
     * @see Extension::ext_clear_prefix
     */
    void ext_clear_prefix(runtime::WasmPointer prefix_data,
                          runtime::SizeType prefix_length);

    /**
     * @see Extension::ext_clear_storage
     */
    void ext_clear_storage(runtime::WasmPointer key_data,
                           runtime::SizeType key_length);

    /**
     * @see Extension::ext_exists_storage
     */
    runtime::SizeType ext_exists_storage(runtime::WasmPointer key_data,
                                         runtime::SizeType key_length) const;

    /**
     * @see Extension::ext_get_allocated_storage
     */
    runtime::WasmPointer ext_get_allocated_storage(
        runtime::WasmPointer key_data,
        runtime::SizeType key_length,
        runtime::WasmPointer len_ptr);

    /**
     * @see Extension::ext_get_storage_into
     */
    runtime::SizeType ext_get_storage_into(runtime::WasmPointer key_data,
                                           runtime::SizeType key_length,
                                           runtime::WasmPointer value_data,
                                           runtime::SizeType value_length,
                                           runtime::SizeType value_offset);

    /**
     * @see Extension::ext_set_storage
     */
    void ext_set_storage(runtime::WasmPointer key_data,
                         runtime::SizeType key_length,
                         runtime::WasmPointer value_data,
                         runtime::SizeType value_length);

    // -------------------------Trie operations--------------------------

    /**
     * @see Extension::ext_blake2_256_enumerated_trie_root
     */
    void ext_blake2_256_enumerated_trie_root(runtime::WasmPointer values_data,
                                             runtime::WasmPointer lengths_data,
                                             runtime::SizeType values_num,
                                             runtime::WasmPointer result);

    /**
     * @see Extension::ext_storage_changes_root
     */
    runtime::SizeType ext_storage_changes_root(runtime::WasmPointer parent_hash,
                                               runtime::WasmPointer result);

    /**
     * @see Extension::ext_storage_root
     */
    void ext_storage_root(runtime::WasmPointer result) const;

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
                                        runtime::SizeType offset,
                                        runtime::SizeType max_length) const;

    std::shared_ptr<storage::trie::TrieBatch> storage_batch_;
    std::shared_ptr<runtime::WasmMemory> memory_;
    std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker_;
    common::Logger logger_;

    constexpr static auto kDefaultLoggerTag = "WASM Runtime [StorageExtension]";
  };
}  // namespace kagome::extensions

#endif  // KAGOME_STORAGE_EXTENSIONS_EXTENSION_HPP
