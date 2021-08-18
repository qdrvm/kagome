/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_WAVM_INTRINSIC_RESOLVER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_WAVM_INTRINSIC_RESOLVER_MOCK_HPP

#include "runtime/wavm/intrinsic_resolver.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime::wavm {

  class IntrinsicResolverMock : public IntrinsicResolver {
   public:
    MOCK_METHOD4(resolve, bool (const std::string &moduleName,
                              const std::string &exportName,
                              WAVM::IR::ExternType type,
                     WAVM::Runtime::Object *&outObject));
    MOCK_CONST_METHOD0(getMemory, std::shared_ptr<runtime::Memory> ());
    MOCK_CONST_METHOD0(clone, std::unique_ptr<IntrinsicResolver>());
  };
}

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_WAVM_INTRINSIC_RESOLVER_MOCK_HPP
