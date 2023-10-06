/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_properties_cache.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class RuntimePropertiesCacheMock : public RuntimePropertiesCache {
   public:
    MOCK_METHOD(outcome::result<primitives::Version>,
                getVersion,
                (const common::Hash256 &,
                 std::function<outcome::result<primitives::Version>()>),
                (override));

    MOCK_METHOD(outcome::result<primitives::OpaqueMetadata>,
                getMetadata,
                (const common::Hash256 &,
                 std::function<outcome::result<primitives::OpaqueMetadata>()>),
                (override));
  };

}  // namespace kagome::runtime
