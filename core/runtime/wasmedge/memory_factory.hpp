/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_WASMEDGE_MEMORY_FACTORY_HPP
#define KAGOME_WASMEDGE_MEMORY_FACTORY_HPP

#include "runtime/wasmedge/memory_impl.hpp"

namespace kagome::runtime::wasmedge {

  class WasmedgeMemoryFactory {
   public:
    virtual ~WasmedgeMemoryFactory() = default;

    std::unique_ptr<MemoryImpl> make(
        WasmEdge_MemoryInstanceContext *memory) const;
  };

}  // namespace kagome::runtime::wasmedge

#endif  // KAGOME_WASMEDGE_MEMORY_FACTORY_HPP
