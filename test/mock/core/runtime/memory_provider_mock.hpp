/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>
#include "runtime/memory_provider.hpp"

namespace kagome::runtime {

  class MemoryProviderMock final : public MemoryProvider {
   public:
    MOCK_METHOD(std::optional<std::reference_wrapper<Memory>>,
                getCurrentMemory,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<void>,
                resetMemory,
                (const MemoryConfig &),
                (override));
  };

}  // namespace kagome::runtime
