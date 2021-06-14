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

namespace kagome::runtime::wavm {

  class IntrinsicResolver : public WAVM::Runtime::Resolver {
   public:
    virtual ~IntrinsicResolver() = default;

    virtual bool resolve(const std::string &moduleName,
                         const std::string &exportName,
                         WAVM::IR::ExternType type,
                         WAVM::Runtime::Object *&outObject) = 0;

    virtual std::unique_ptr<IntrinsicResolver> clone() const = 0;
  };
}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_INTRINSIC_RESOLVER_HPP
