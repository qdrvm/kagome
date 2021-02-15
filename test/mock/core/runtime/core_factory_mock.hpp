/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_FACTORY_MOCK_HPP
#define KAGOME_CORE_FACTORY_MOCK_HPP

#include "runtime/core_factory.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class CoreFactoryMock : public CoreFactory {
   public:
    ~CoreFactoryMock() override = default;

    MOCK_METHOD1(createWithCode,
                 std::unique_ptr<Core>(std::shared_ptr<WasmProvider> wasm_provider));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_FACTORY_MOCK_HPP
