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
        logger_{log::createLogger("Binaryen Memory", "binaryen")} {
  }

  std::optional<WasmSize> MemoryImpl::pagesMax() const {
    return memory_->pagesMax();
  }

  outcome::result<BytesOut> MemoryImpl::view(WasmPointer ptr,
                                             WasmSize size) const {
    if (not memoryCheck(ptr, size, this->size())) {
      return MemoryError::ERROR;
    }
    return memory_->view(ptr, size);
  }
}  // namespace kagome::runtime::binaryen
