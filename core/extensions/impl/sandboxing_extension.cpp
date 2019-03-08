/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>

#include "extensions/impl/sandboxing_extension.hpp"

namespace extensions {
  void SandboxingExtension::ext_sandbox_instance_teardown(
      uint32_t instance_idx) {
    std::terminate();
  }

  uint32_t SandboxingExtension::ext_sandbox_instantiate(
      SandoxDispatchFuncType dispatch_func, const uint8_t *wasm_ptr,
      size_t wasm_length, const uint8_t *imports_ptr, size_t imports_length,
      size_t state) {
    std::terminate();
  }

  uint32_t SandboxingExtension::ext_sandbox_invoke(
      uint32_t instance_idx, const uint8_t *export_ptr, size_t export_len,
      const uint8_t *args_ptr, size_t args_len, uint8_t *return_val_ptr,
      size_t return_val_len, size_t state) {
    std::terminate();
  }

  uint32_t SandboxingExtension::ext_sandbox_memory_get(uint32_t memory_idx,
                                                       uint32_t offset,
                                                       uint8_t *buf_ptr,
                                                       size_t buf_length) {
    std::terminate();
  }

  uint32_t SandboxingExtension::ext_sandbox_memory_new(uint32_t initial,
                                                       uint32_t maximum) {
    std::terminate();
  }

  uint32_t SandboxingExtension::ext_sandbox_memory_set(uint32_t memory_idx,
                                                       uint32_t offset,
                                                       const uint8_t *val_ptr,
                                                       size_t val_len) {
    std::terminate();
  }

  void SandboxingExtension::ext_sandbox_memory_teardown(uint32_t memory_idx) {
    std::terminate();
  }
}  // namespace extensions
