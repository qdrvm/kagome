/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SPACED_STORAGE_MOCK_HPP
#define KAGOME_SPACED_STORAGE_MOCK_HPP

#include <gmock/gmock.h>

#include "storage/spaced_storage.hpp"

namespace kagome::storage {

  class SpacedStorageMock : public SpacedStorage {
   public:
    MOCK_METHOD(std::shared_ptr<BufferStorage>, getSpace, (Space), (override));
  };

}  // namespace kagome::storage

#endif  // KAGOME_SPACED_STORAGE_MOCK_HPP
