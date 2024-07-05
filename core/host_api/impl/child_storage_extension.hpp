/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

#include "common/buffer.hpp"
#include "log/logger.hpp"
#include "primitives/kill_storage_result.hpp"
#include "runtime/types.hpp"
#include "storage/trie/types.hpp"

namespace kagome::runtime {
  class MemoryProvider;
  class TrieStorageProvider;
  class Memory;
}  // namespace kagome::runtime

namespace kagome::host_api {
  /**
   * Implements HostApi functions related to storage
   */
  class ChildStorageExtension {
   public:
    ChildStorageExtension(
        std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
        std::shared_ptr<const runtime::MemoryProvider> memory_provider);

    // ---------------------------- Child Storage ----------------------------

    /**
     * @see HostApi::ext_default_child_storage_set_version_1
     */
    void ext_default_child_storage_set_version_1(
        runtime::WasmSpan child_storage_key,
        runtime::WasmSpan key,
        runtime::WasmSpan value);

    /**
     * @see HostApi::ext_default_child_storage_get_version_1
     */
    runtime::WasmSpan ext_default_child_storage_get_version_1(
        runtime::WasmSpan child_storage_key, runtime::WasmSpan key) const;

    /**
     * @see HostApi::ext_default_child_storage_clear_version_1
     */
    void ext_default_child_storage_clear_version_1(
        runtime::WasmSpan child_storage_key, runtime::WasmSpan key);

    /**
     * @see HostApi::ext_default_child_storage_next_key_version_1
     */
    runtime::WasmSpan ext_default_child_storage_next_key_version_1(
        runtime::WasmSpan child_storage_key, runtime::WasmSpan key) const;

    /**
     * @see HostApi::ext_default_child_storage_root_version_1
     */
    Hash256 ext_default_child_storage_root_version_1(BufferView child) const;

    /**
     * @see HostApi::ext_default_child_storage_root_version_2
     */
    Hash256 ext_default_child_storage_root_version_2(
        BufferView child, storage::trie::StateVersion version) const;

    /**
     * @see HostApi::ext_default_child_storage_clear_prefix_version_1
     */
    void ext_default_child_storage_clear_prefix_version_1(BufferView child,
                                                          BufferView prefix);

    /**
     * @see HostApi::ext_default_child_storage_clear_prefix_version_2
     */
    KillStorageResult ext_default_child_storage_clear_prefix_version_2(
        BufferView child, BufferView prefix, ClearPrefixLimit limit);

    /**
     * @see HostApi::ext_default_child_storage_read_version_1
     */
    runtime::WasmSpan ext_default_child_storage_read_version_1(
        runtime::WasmSpan child_storage_key,
        runtime::WasmSpan key,
        runtime::WasmSpan value_out,
        runtime::WasmOffset offset) const;

    /**
     * @see HostApi::ext_default_child_storage_exists_version_1
     */
    int32_t ext_default_child_storage_exists_version_1(
        runtime::WasmSpan child_storage_key, runtime::WasmSpan key) const;

    /**
     * @see HostApi::ext_default_child_storage_storage_kill_version_1
     */
    void ext_default_child_storage_storage_kill_version_1(BufferView child);

    /**
     * @see HostApi::ext_default_child_storage_storage_kill_version_3
     */
    KillStorageResult ext_default_child_storage_storage_kill_version_3(
        BufferView child, ClearPrefixLimit limit);

   private:
    std::shared_ptr<runtime::TrieStorageProvider> storage_provider_;
    std::shared_ptr<const runtime::MemoryProvider> memory_provider_;
    log::Logger logger_;

    static constexpr auto kDefaultLoggerTag =
        "WASM Runtime [ChildStorageExtension]";

    template <typename R, typename F, typename... Args>
    outcome::result<R> executeOnConstChildStorage(
        const common::Buffer &child_storage_key, F func, Args &&...args) const;

    template <typename R, typename F, typename... Args>
    outcome::result<R> executeOnMutChildStorage(
        const common::Buffer &child_storage_key, F func, Args &&...args) const;
  };

}  // namespace kagome::host_api
