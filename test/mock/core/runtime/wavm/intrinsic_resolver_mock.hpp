/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/wavm/intrinsic_resolver.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime::wavm {

  class IntrinsicResolverMock : public IntrinsicResolver {
   public:
    MOCK_METHOD(bool,
                resolve,
                (const std::string &moduleName,
                 const std::string &exportName,
                 WAVM::IR::ExternType type,
                 WAVM::Runtime::Object *&outObject),
                (override));

    MOCK_METHOD(std::shared_ptr<runtime::Memory>,
                getMemory,
                (),
                (const, override));

    MOCK_METHOD(std::unique_ptr<IntrinsicResolver>,
                clone,
                (),
                (const, override));
  };
}  // namespace kagome::runtime::wavm
