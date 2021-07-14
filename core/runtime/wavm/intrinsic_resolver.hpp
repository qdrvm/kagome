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

    /**
     * Resolves a dependency between a Runtime module and the Host API module
     * @param moduleName the name of the module which requires the dependency
     * @param exportName the name of the dependency entity
     * @param type type of the dependency (e.g. memory or a function)
     * @param outObject the resolved dependency
     * @return true if resolved successfully, false otherwise
     */
    virtual bool resolve(const std::string &moduleName,
                         const std::string &exportName,
                         WAVM::IR::ExternType type,
                         WAVM::Runtime::Object *&outObject) = 0;

  };
}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_INTRINSIC_RESOLVER_HPP
