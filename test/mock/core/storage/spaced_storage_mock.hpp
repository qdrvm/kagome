/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "storage/spaced_storage.hpp"

namespace kagome::storage {

  class SpacedStorageMock : public SpacedStorage {
   public:
    MOCK_METHOD(std::shared_ptr<BufferBatchableStorage>,
                getSpace,
                (Space),
                (override));

    MOCK_METHOD(std::unique_ptr<BufferSpacedBatch>,
                createBatch,
                (),
                (override));
  };

}  // namespace kagome::storage
