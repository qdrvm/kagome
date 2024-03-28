/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/memory_impl.hpp"

#include "runtime/common/memory_allocator.hpp"
#include "runtime/common/memory_error.hpp"
#include "runtime/memory_check.hpp"
#include "runtime/ptr_size.hpp"

namespace kagome::runtime::binaryen {

  MemoryImpl::MemoryImpl(RuntimeExternalInterface::InternalMemory *memory,
                         const MemoryConfig &config)
      : memory_{memory},
        allocator_{std::make_unique<MemoryAllocator>(*this, config)},
        logger_{log::createLogger("Binaryen Memory", "binaryen")} {
    // TODO(Harrm): #1714 temporary fix because binaryen doesn't recognize
    // our memory resizes from our allocator
    memory_->resize(kInitialMemorySize);
  }

  std::optional<WasmSize> MemoryImpl::pagesMax() const {
    return memory_->pagesMax();
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
    return memory_->view(ptr, size);
  }
}  // namespace kagome::runtime::binaryen
