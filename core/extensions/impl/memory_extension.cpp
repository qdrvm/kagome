#include <utility>

#include <utility>

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>

#include "extensions/impl/memory_extension.hpp"

namespace kagome::extensions {
  MemoryExtension::MemoryExtension(common::Logger logger)
      : logger_(std::move(logger)) {}

  MemoryExtension::MemoryExtension(std::shared_ptr<runtime::WasmMemory> memory,
                                   common::Logger logger)
      : memory_(std::move(memory)), logger_(std::move(logger)) {}

  int32_t MemoryExtension::ext_malloc(uint32_t size) {
    return memory_->allocate(size);
  }

  void MemoryExtension::ext_free(int32_t ptr) {
    auto opt_size = memory_->deallocate(ptr);
    if (not opt_size) {
      logger_->info(
          "Ptr {} does not point to any memory chunk in wasm memory. Nothing "
          "deallocated",
          ptr);
    }
  }
}  // namespace kagome::extensions
