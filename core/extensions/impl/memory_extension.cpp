#include <utility>

#include <utility>

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>

#include "extensions/impl/memory_extension.hpp"

namespace kagome::extensions {
  MemoryExtension::MemoryExtension(std::shared_ptr<runtime::WasmMemory> memory)
      : memory_(std::move(memory)),
        logger_{common::createLogger(kDefaultLoggerTag)} {
    BOOST_ASSERT_MSG(memory_ != nullptr, "memory is nullptr");
  }

  runtime::WasmPointer MemoryExtension::ext_malloc(runtime::WasmSize size) {
    return memory_->allocate(size);
  }

  void MemoryExtension::ext_free(runtime::WasmPointer ptr) {
    auto opt_size = memory_->deallocate(ptr);
    if (not opt_size) {
      logger_->info(
          "Ptr {} does not point to any memory chunk in wasm memory. Nothing "
          "deallocated",
          ptr);
    }
  }
}  // namespace kagome::extensions
