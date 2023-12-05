/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_code_provider.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class WasmProviderMock : public RuntimeCodeProvider {
   public:
    MOCK_METHOD(const common::Buffer &, getStateCode, (), (const, override));
  };

}  // namespace kagome::runtime
