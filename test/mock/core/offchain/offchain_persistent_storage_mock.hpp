/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "offchain/offchain_persistent_storage.hpp"

namespace kagome::offchain {

  class OffchainPersistentStorageMock : public OffchainPersistentStorage {
   public:
    MOCK_METHOD(outcome::result<void>,
                set,
                (const common::BufferView &, common::Buffer),
                (override));

    MOCK_METHOD(outcome::result<void>,
                clear,
                (const common::BufferView &),
                (override));

    MOCK_METHOD(outcome::result<bool>,
                compare_and_set,
                (const common::BufferView &,
                 const std::optional<common::BufferView> &,
                 common::Buffer),
                (override));

    MOCK_METHOD(outcome::result<common::Buffer>,
                get,
                (const common::BufferView &),
                (override));
  };

}  // namespace kagome::offchain
