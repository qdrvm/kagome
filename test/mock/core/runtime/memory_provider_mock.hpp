/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_MEMORY_PROVIDER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_MEMORY_PROVIDER_MOCK_HPP

#include <gmock/gmock.h>
#include "runtime/memory_provider.hpp"

namespace kagome::runtime {

  class MemoryProviderMock final : public MemoryProvider {
   public:
    MOCK_CONST_METHOD0(getCurrentMemory, boost::optional<Memory&>());
    MOCK_METHOD1(resetMemory, outcome::result<void> (WasmSize));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_MEMORY_PROVIDER_MOCK_HPP
