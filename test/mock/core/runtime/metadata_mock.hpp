/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/metadata.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class MetadataMock : public Metadata {
   public:
    MOCK_METHOD(outcome::result<Metadata::OpaqueMetadata>,
                metadata,
                (const primitives::BlockHash &),
                (override));
  };

}  // namespace kagome::runtime
