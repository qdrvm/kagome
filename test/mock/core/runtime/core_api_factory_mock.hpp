/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/core_api_factory.hpp"

#include <gmock/gmock.h>

#include "runtime/executor.hpp"
#include "runtime/runtime_api/core.hpp"

namespace kagome::runtime {

  class CoreApiFactoryMock : public CoreApiFactory {
   public:
    MOCK_METHOD(std::unique_ptr<Core>,
                make,
                (std::shared_ptr<const crypto::Hasher> hasher,
                 const std::vector<uint8_t> &runtime_code),
                (const, override));
  };

}  // namespace kagome::runtime
