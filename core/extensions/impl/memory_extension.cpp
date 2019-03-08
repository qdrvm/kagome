/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>

#include "extensions/impl/memory_extension.hpp"

namespace extensions {
  uint8_t *MemoryExtension::ext_malloc(uint32_t size) {
    std::terminate();
  }

  void MemoryExtension::ext_free(uint8_t *ptr) {
    std::terminate();
  }
}  // namespace extensions
