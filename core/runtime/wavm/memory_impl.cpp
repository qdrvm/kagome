/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/memory_impl.hpp"

#include "log/trace_macros.hpp"
#include "runtime/common/memory_allocator.hpp"
#include "runtime/common/memory_error.hpp"
#include "runtime/memory_check.hpp"

namespace kagome::runtime::wavm {

  MemoryImpl::MemoryImpl(WAVM::Runtime::Memory *memory,
                         const MemoryConfig &config)
      : allocator_{std::make_unique<MemoryAllocator>(*this, config)},
        memory_{memory},
        logger_{log::createLogger("WAVM Memory", "wavm")} {
    BOOST_ASSERT(memory_);
    BOOST_ASSERT(allocator_);
    resize(kInitialMemorySize);
  }

  WasmPointer MemoryImpl::allocate(WasmSize size) {
    return allocator_->allocate(size);
  }

  void MemoryImpl::deallocate(WasmPointer ptr) {
    return allocator_->deallocate(ptr);
  }

  outcome::result<BytesOut> MemoryImpl::view(WasmPointer ptr,
                                             WasmSize size) const {
    if (not memoryCheck(ptr, size, this->size())) {
      return MemoryError::ERROR;
    }
    auto raw = WAVM::Runtime::getValidatedMemoryOffsetRange(memory_, ptr, size);
    return BytesOut{raw, size};
  }
}  // namespace kagome::runtime::wavm
