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
    MOCK_METHOD(outcome::result<std::unique_ptr<RestrictedCore>>,
                make,
                (BufferView, std::shared_ptr<TrieStorageProvider>),
                (const, override));
  };

}  // namespace kagome::runtime
