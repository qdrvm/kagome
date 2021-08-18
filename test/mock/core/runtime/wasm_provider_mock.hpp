/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_WASM_PROVIDER_MOCK
#define KAGOME_TEST_MOCK_CORE_RUNTIME_WASM_PROVIDER_MOCK

#include "runtime/runtime_code_provider.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class WasmProviderMock: public RuntimeCodeProvider {
   public:
    MOCK_CONST_METHOD0(getStateCode, const common::Buffer &());
  };

}

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_WASM_PROVIDER_MOCK
