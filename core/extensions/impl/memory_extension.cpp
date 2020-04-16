#include <utility>

#include <utility>

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <exception>

#include "extensions/impl/memory_extension.hpp"

namespace kagome::extensions {
  MemoryExtension::MemoryExtension(std::shared_ptr<runtime::WasmMemory> memory)
      : memory_(std::move(memory)),
        logger_{common::createLogger(kDefaultLoggerTag)} {
    BOOST_ASSERT_MSG(memory_ != nullptr, "memory is nullptr");
  }

  runtime::WasmPointer MemoryExtension::ext_malloc(runtime::SizeType size) {
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
