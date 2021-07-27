/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_INTRINSIC_RESOLVER_HPP
#define KAGOME_CORE_RUNTIME_WAVM_INTRINSIC_RESOLVER_HPP

#include <WAVM/Runtime/Linker.h>

#include <memory>
#include <string>

#include "runtime/memory_provider.hpp"

namespace WAVM::Intrinsics {
  struct Function;
}

namespace kagome::runtime::wavm {

  /**
   * Resolver of the dependencies between the Runtime module and the intrinsic
   * module, which stores Host API methods
   * The Runtime module links to the native memory and Host API methods from the
   * intrinsic module
   */
  class IntrinsicResolver : public WAVM::Runtime::Resolver {
   public:
    virtual ~IntrinsicResolver() = default;
  };
}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_INTRINSIC_RESOLVER_HPP
