/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>

#include <boost/assert.hpp>

#include "host_api/impl/memory_extension.hpp"
#include "log/trace_macros.hpp"
#include "runtime/memory_provider.hpp"

namespace kagome::host_api {
  MemoryExtension::MemoryExtension(
      std::shared_ptr<const runtime::MemoryProvider> memory_provider)
      : memory_provider_(std::move(memory_provider)),
        logger_{log::createLogger("MemoryExtension", "memory_extension")} {
    BOOST_ASSERT_MSG(memory_provider_ != nullptr, "memory provider is nullptr");
    SL_DEBUG(logger_,
             "Memory extension {} initialized with memory provider {}",
             fmt::ptr(this),
             fmt::ptr(memory_provider_));
  }

  runtime::WasmPointer MemoryExtension::ext_allocator_malloc_version_1(
      runtime::WasmSize size) {
    auto res = memory_provider_->getCurrentMemory()->get().allocate(size);
    SL_TRACE_FUNC_CALL(logger_, res, size);
    return res;
  }

  void MemoryExtension::ext_allocator_free_version_1(runtime::WasmPointer ptr) {
    memory_provider_->getCurrentMemory()->get().deallocate(ptr);
    SL_TRACE_FUNC_CALL(logger_, ptr);
  }
}  // namespace kagome::host_api
