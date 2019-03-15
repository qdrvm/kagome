/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>

#include "extensions/impl/memory_extension.hpp"
#include "memory_extension.hpp"

namespace kagome::extensions {
  MemoryExtension::MemoryExtension()
      : logger_{common::createLogger("WASM Runtime [MemoryExtension]")} {}

  MemoryExtension::MemoryExtension(std::shared_ptr<runtime::Memory> memory)
      : memory_(std::move(memory)) {}

  int32_t MemoryExtension::ext_malloc(uint32_t size) {
    auto opt_ptr = memory_->allocate(size);
    if (opt_ptr.has_value()) {
      return *opt_ptr;
    }
    // FIXME: not clear from specification what to return when memopry cannot be
    // allocated. For now return -1
    logger_->info(
        "There is no space in wasm memory to allocate memory of size {}", size);
    return -1;
  }

  void MemoryExtension::ext_free(int32_t ptr) {
    auto opt_size = memory_->deallocate(ptr);
    if (not opt_size) {
      logger_->info(
          "Ptr {} does not point to any memory chunk in wasm memory. Nothig "
          "deallocated",
          ptr);
    }
  }
}  // namespace kagome::extensions
