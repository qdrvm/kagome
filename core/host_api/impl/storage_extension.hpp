/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HOST_API_STORAGE_EXTENSION_HPP
#define KAGOME_HOST_API_STORAGE_EXTENSION_HPP

#include <cstdint>

#include "common/buffer_or_view.hpp"
#include "log/logger.hpp"
#include "runtime/types.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

namespace kagome::runtime {
  class MemoryProvider;
  class TrieStorageProvider;
}  // namespace kagome::runtime

namespace kagome::host_api {
  /**
   * Implements HostApi functions related to storage
   */
  class StorageExtension {
   public:
    StorageExtension(
        std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
        std::shared_ptr<const runtime::MemoryProvider> memory_provider);

    void reset();

    // -------------------------Trie operations--------------------------

    /**
     * @see HostApi::ext_storage_read_version_1
     */
    runtime::WasmSpan ext_storage_read_version_1(runtime::WasmSpan key,
                                                 runtime::WasmSpan value_out,
                                                 runtime::WasmOffset offset);

    // ------------------------ VERSION 1 ------------------------

    /**
     * @see HostApi::ext_storage_set_version_1
     */
    void ext_storage_set_version_1(runtime::WasmSpan key,
                                   runtime::WasmSpan value);

    /**
     * @see HostApi::ext_storage_get_version_1
     */
    runtime::WasmSpan ext_storage_get_version_1(runtime::WasmSpan key);

    /**
     * @see HostApi::ext_storage_clear_version_1
     */
    void ext_storage_clear_version_1(runtime::WasmSpan key_data);

    /**
     * @see HostApi::ext_storage_exists_version_1
     */
    runtime::WasmSize ext_storage_exists_version_1(
        runtime::WasmSpan key_data) const;

    /**
     * @see HostApi::ext_storage_clear_prefix_version_1
     */
    void ext_storage_clear_prefix_version_1(runtime::WasmSpan prefix);

    /**
     * @see HostApi::ext_storage_clear_prefix_version_2
     */
    runtime::WasmSpan ext_storage_clear_prefix_version_2(
        runtime::WasmSpan prefix, runtime::WasmSpan limit);

    /**
     * @see HostApi::ext_storage_root_version_1
     */
    runtime::WasmSpan ext_storage_root_version_1();

    /**
     * @see HostApi::ext_storage_root_version_2
     */
    runtime::WasmSpan ext_storage_root_version_2(
        runtime::WasmI32 state_version);

    /**
     * @see HostApi::ext_storage_changes_root_version_1
     */
    runtime::WasmSpan ext_storage_changes_root_version_1(
        runtime::WasmSpan parent_hash);

    /**
     * @see HostApi::ext_storage_next_key
     */
    runtime::WasmSpan ext_storage_next_key_version_1(
        runtime::WasmSpan key) const;

    /**
     * @see HostApi::ext_storage_append_version_1
     */
    void ext_storage_append_version_1(runtime::WasmSpan key,
                                      runtime::WasmSpan value) const;

    /**
     * @see HostApi::ext_storage_start_transaction_version_1
     */
    void ext_storage_start_transaction_version_1();

    /**
     * @see HostApi::ext_storage_commit_transaction_version_1
     */
    void ext_storage_commit_transaction_version_1();

    /**
     * @see HostApi::ext_storage_rollback_transaction_version_1
     */
    void ext_storage_rollback_transaction_version_1();

    /**
     * @see HostApi::ext_trie_blake2_256_root_version_1
     */
    runtime::WasmPointer ext_trie_blake2_256_root_version_1(
        runtime::WasmSpan values_data);

    /**
     * @see HostApi::ext_trie_blake2_256_ordered_root_version_1
     */
    runtime::WasmPointer ext_trie_blake2_256_ordered_root_version_1(
        runtime::WasmSpan values_data);

    /**
     * @see HostApi::ext_trie_blake2_256_ordered_root_version_2
     */
    runtime::WasmPointer ext_trie_blake2_256_ordered_root_version_2(
        runtime::WasmSpan values_data, runtime::WasmI32 state_version);

   private:
    /**
     * Find the value by given key and the return the part of it starting from
     * given offset
     *
     * @param key Buffer representation of the key
     * @return result containing Buffer with the value
     */
    outcome::result<std::optional<common::BufferOrView>> get(
        const common::BufferView &key) const;

    /**
     * Read key in form of [ptr; size] and load its value
     * from memory into buffer
     *
     * @param key representation by [ptr; size]
     * @return result containing Buffer with the key
     */
    common::Buffer loadKey(runtime::WasmSpan key) const;
    /**
     * @return error if any, a key if the next key exists
     * none otherwise
     */
    outcome::result<std::optional<common::Buffer>> getStorageNextKey(
        const common::Buffer &key) const;

    runtime::WasmSpan clearPrefix(common::BufferView prefix,
                                  std::optional<uint32_t> limit);

    std::shared_ptr<runtime::TrieStorageProvider> storage_provider_;
    std::shared_ptr<const runtime::MemoryProvider> memory_provider_;
    storage::trie::PolkadotCodec codec_;
    log::Logger logger_;

    constexpr static auto kDefaultLoggerTag = "WASM Runtime [StorageExtension]";
  };

}  // namespace kagome::host_api

#endif  // KAGOME_HOST_API_STORAGE_EXTENSION_HPP
