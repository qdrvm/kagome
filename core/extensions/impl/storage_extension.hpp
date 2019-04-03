/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_EXTENSION_HPP
#define KAGOME_STORAGE_EXTENSION_HPP

#include <cstdint>

#include "common/logger.hpp"
#include "runtime/wasm_memory.hpp"
#include "storage/merkle/codec.hpp"
#include "storage/merkle/trie_db.hpp"

namespace kagome::extensions {
  /**
   * Implements extension functions related to storage
   */
  class StorageExtension {
   public:
    explicit StorageExtension(
        std::shared_ptr<storage::merkle::TrieDb> db,
        std::shared_ptr<runtime::WasmMemory> memory,
        std::shared_ptr<storage::merkle::Codec> codec,
        common::Logger logger = common::createLogger(kDefaultLoggerTag));

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
                                         runtime::SizeType key_length);

    /**
     * @see Extension::ext_get_allocated_storage
     */
    runtime::WasmPointer ext_get_allocated_storage(
        runtime::WasmPointer key_data, runtime::SizeType key_length,
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
                                             runtime::WasmPointer lens_data,
                                             runtime::SizeType lens_length,
                                             runtime::WasmPointer result);

    /**
     * @see Extension::ext_storage_changes_root
     */
    runtime::SizeType ext_storage_changes_root(
        runtime::WasmPointer parent_hash_data,
        runtime::SizeType parent_hash_len, runtime::SizeType parent_num,
        runtime::WasmPointer result);

    /**
     * @see Extension::ext_storage_root
     */
    void ext_storage_root(runtime::WasmPointer result) const;

    // -------------------------Child storage--------------------------

    /**
     * @see Extension::ext_child_storage_root
     */
    runtime::WasmPointer ext_child_storage_root(
        runtime::WasmPointer storage_key_data,
        runtime::WasmPointer storage_key_length, runtime::WasmPointer written);

    /**
     * @see Extension::ext_clear_child_storage
     */
    void ext_clear_child_storage(runtime::WasmPointer storage_key_data,
                                 runtime::WasmPointer storage_key_length,
                                 runtime::WasmPointer key_data,
                                 runtime::WasmPointer key_length);

    /**
     * @see Extension::ext_exists_child_storage
     */
    runtime::SizeType ext_exists_child_storage(
        runtime::WasmPointer storage_key_data,
        runtime::WasmPointer storage_key_length, runtime::WasmPointer key_data,
        runtime::WasmPointer key_length);

    /**
     * @see Extension::ext_get_allocated_child_storage
     */
    runtime::WasmPointer ext_get_allocated_child_storage(
        runtime::WasmPointer storage_key_data,
        runtime::WasmPointer storage_key_length, runtime::WasmPointer key_data,
        runtime::WasmPointer key_length, runtime::WasmPointer written);

    /**
     * @see Extension::ext_get_child_storage_into
     */
    runtime::SizeType ext_get_child_storage_into(
        runtime::WasmPointer storage_key_data,
        runtime::WasmPointer storage_key_length, runtime::WasmPointer key_data,
        runtime::WasmPointer key_length, runtime::WasmPointer value_data,
        runtime::SizeType value_length, runtime::SizeType value_offset);

    /**
     * @see Extension::ext_kill_child_storage
     */
    void ext_kill_child_storage(runtime::WasmPointer storage_key_data,
                                runtime::SizeType storage_key_length);

    /**
     * @see Extension::ext_set_child_storage
     */
    void ext_set_child_storage(runtime::WasmPointer storage_key_data,
                               runtime::SizeType storage_key_length,
                               runtime::WasmPointer key_data,
                               runtime::SizeType key_length,
                               runtime::WasmPointer value_data,
                               runtime::SizeType value_length);

   private:
    /**
     * Find the value by given key and the return the part of it starting from
     * given offset
     *
     * @param key Buffer representation of the key
     * @param offset SizeType pointing to the beginning of the value
     * @param max_length SizeType defining the maximum possible length of the
     * returned result
     * @return optional containing Buffer with the part of the value, or none in
     * case value by give key dopes not exist
     */
    std::optional<common::Buffer> get(const common::Buffer &key,
                                      runtime::SizeType offset,
                                      runtime::SizeType max_length) const;

    std::shared_ptr<storage::merkle::TrieDb> db_;
    std::shared_ptr<runtime::WasmMemory> memory_;
    std::shared_ptr<storage::merkle::Codec> codec_;
    common::Logger logger_;

    constexpr static auto kDefaultLoggerTag = "WASM Runtime [StorageExtension]";
  };
}  // namespace kagome::extensions

#endif  // KAGOME_STORAGE_EXTENSION_HPP
