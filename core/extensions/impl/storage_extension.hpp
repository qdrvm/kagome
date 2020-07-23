/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_EXTENSION_HPP
#define KAGOME_STORAGE_EXTENSION_HPP

#include "runtime/wasm_memory.hpp"

namespace kagome::extensions {
  /**
   * Implements extension functions related to storage
   */
  class StorageExtension {
   public:
    virtual ~StorageExtension() = default;

    // -------------------------Data storage--------------------------
    /**
     * @see Extension::ext_clear_prefix
     */
    virtual void ext_clear_prefix(runtime::WasmPointer prefix_data,
                                  runtime::WasmSize prefix_length) = 0;

    /**
     * @see Extension::ext_clear_storage
     */
    virtual void ext_clear_storage(runtime::WasmPointer key_data,
                                   runtime::WasmSize key_length) = 0;

    /**
     * @see Extension::ext_exists_storage
     */
    virtual runtime::WasmSize ext_exists_storage(
        runtime::WasmPointer key_data, runtime::WasmSize key_length) const = 0;

    /**
     * @see Extension::ext_get_allocated_storage
     */
    virtual runtime::WasmPointer ext_get_allocated_storage(
        runtime::WasmPointer key_data,
        runtime::WasmSize key_length,
        runtime::WasmPointer len_ptr) = 0;

    /**
     * @see Extension::ext_get_storage_into
     */
    virtual runtime::WasmSize ext_get_storage_into(
        runtime::WasmPointer key_data,
        runtime::WasmSize key_length,
        runtime::WasmPointer value_data,
        runtime::WasmSize value_length,
        runtime::WasmSize value_offset) = 0;

    /**
     * @see Extension::ext_storage_read_version_1
     */
    virtual runtime::WasmSpan ext_storage_read_version_1(
        runtime::WasmSpan key,
        runtime::WasmSpan value_out,
        runtime::WasmOffset offset) = 0;

    /**
     * @see Extension::ext_set_storage
     */
    virtual void ext_set_storage(runtime::WasmPointer key_data,
                                 runtime::WasmSize key_length,
                                 runtime::WasmPointer value_data,
                                 runtime::WasmSize value_length) = 0;

    // -------------------------Trie operations--------------------------

    /**
     * @see Extension::ext_blake2_256_enumerated_trie_root
     */
    virtual void ext_blake2_256_enumerated_trie_root(
        runtime::WasmPointer values_data,
        runtime::WasmPointer lengths_data,
        runtime::WasmSize values_num,
        runtime::WasmPointer result) = 0;

    /**
     * @see Extension::ext_storage_changes_root
     */
    virtual runtime::WasmPointer ext_storage_changes_root(
        runtime::WasmPointer parent_hash, runtime::WasmPointer result) = 0;

    /**
     * @see Extension::ext_storage_root
     */
    virtual void ext_storage_root(runtime::WasmPointer result) const = 0;

    // ------------------------ VERSION 1 ------------------------

    /**
     * @see Extension::ext_storage_set_version_1
     */
    virtual void ext_storage_set_version_1(runtime::WasmSpan key,
                                           runtime::WasmSpan value) = 0;

    /**
     * @see Extension::ext_storage_get_version_1
     */
    virtual runtime::WasmSpan ext_storage_get_version_1(
        runtime::WasmSpan key) = 0;

    /**
     * @see Extension::ext_storage_clear_version_1
     */
    virtual void ext_storage_clear_version_1(runtime::WasmSpan key_data) = 0;

    /**
     * @see Extension::ext_storage_exists_version_1
     */
    virtual runtime::WasmSize ext_storage_exists_version_1(
        runtime::WasmSpan key_data) const = 0;

    /**
     * @see Extension::ext_storage_clear_prefix_version_1
     */
    virtual void ext_storage_clear_prefix_version_1(
        runtime::WasmSpan prefix) = 0;

    /**
     * @see Extension::ext_storage_root_version_1
     */
    virtual runtime::WasmPointer ext_storage_root_version_1() const = 0;

    /**
     * @see Extension::ext_storage_changes_root_version_1
     */
    virtual runtime::WasmPointer ext_storage_changes_root_version_1(
        runtime::WasmSpan parent_hash) = 0;

    /**
     * @see Extension::ext_storage_next_key
     */
    virtual runtime::WasmSpan ext_storage_next_key_version_1(
        runtime::WasmSpan key) const = 0;

    /**
     * @see Extension::ext_trie_blake2_256_root_version_1
     */
    virtual runtime::WasmPointer ext_trie_blake2_256_root_version_1(
        runtime::WasmSpan values_data) = 0;

    /**
     * @see Extension::ext_trie_blake2_256_ordered_root_version_1
     */
    virtual runtime::WasmPointer ext_trie_blake2_256_ordered_root_version_1(
        runtime::WasmSpan values_data) = 0;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_STORAGE_EXTENSION_HPP=
