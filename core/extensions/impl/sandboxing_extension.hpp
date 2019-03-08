/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SANDBOXING_EXTENSION_HPP
#define KAGOME_SANDBOXING_EXTENSION_HPP

#include <functional>

namespace extensions {
  /**
   * Implements extension functions related to sandboxing
   */
  class SandboxingExtension {
   public:
    void ext_sandbox_instance_teardown(uint32_t instance_idx);
    uint32_t ext_sandbox_instantiate(
        std::function<uint64_t(const uint8_t *serialized_args,
                               size_t serialized_args_length,
                               size_t state,
                               size_t func_index)> dispatch_func,
        const uint8_t *wasm_ptr,
        size_t wasm_length,
        const uint8_t *imports_ptr,
        size_t imports_length,
        size_t state);
    uint32_t ext_sandbox_invoke(uint32_t instance_idx,
                                const uint8_t *export_ptr,
                                size_t export_len,
                                const uint8_t *args_ptr,
                                size_t args_len,
                                uint8_t *return_val_ptr,
                                size_t return_val_len,
                                size_t state);
    uint32_t ext_sandbox_memory_get(uint32_t memory_idx,
                                    uint32_t offset,
                                    uint8_t *buf_ptr,
                                    size_t buf_length);
    uint32_t ext_sandbox_memory_new(uint32_t initial, uint32_t maximum);
    uint32_t ext_sandbox_memory_set(uint32_t memory_idx,
                                    uint32_t offset,
                                    const uint8_t *val_ptr,
                                    size_t val_len);
    void ext_sandbox_memory_teardown(uint32_t memory_idx);
  };
}  // namespace extensions

#endif  // KAGOME_SANDBOXING_EXTENSION_HPP
