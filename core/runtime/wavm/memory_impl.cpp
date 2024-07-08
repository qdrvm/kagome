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
      : memory_{memory}, logger_{log::createLogger("WAVM Memory", "wavm")} {
    BOOST_ASSERT(memory_);
    resize(kInitialMemorySize);
  }

  std::optional<WasmSize> MemoryImpl::pagesMax() const {
    auto max = WAVM::Runtime::getMemoryType(memory_).size.max;
    return max != UINT64_MAX ? std::make_optional<WasmSize>(max) : std::nullopt;
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
