/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>

#include "extensions/impl/memory_extension.hpp"
#include "memory_extension.hpp"

namespace kagome::extensions {
  MemoryExtension::MemoryExtension(std::shared_ptr<Memory> memory)
      : memory_(std::move(memory)) {}

  int32_t MemoryExtension::ext_malloc(uint32_t size) {
    auto opt_ptr = memory_->allocate(size);
    if (opt_ptr.has_value()) {
      return opt_ptr.value();
    }
    // not clear from specification what to return when memopry cannot be
    // allocated. For now return -1
    return -1;
  }

  void MemoryExtension::ext_free(int32_t ptr) {
    memory_->deallocate(ptr);
  }
}  // namespace kagome::extensions
