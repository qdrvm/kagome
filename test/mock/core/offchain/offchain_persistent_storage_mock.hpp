/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_PERSISTENT_STORAGE_MOCK_HPP
#define KAGOME_OFFCHAIN_PERSISTENT_STORAGE_MOCK_HPP

#include <gmock/gmock.h>

#include "offchain/offchain_persistent_storage.hpp"

namespace kagome::offchain {

  class OffchainPersistentStorageMock : public OffchainPersistentStorage {
   public:
    MOCK_METHOD2(set,
                 outcome::result<void>(const common::BufferView &,
                                       common::Buffer));

    MOCK_METHOD1(clear, outcome::result<void>(const common::BufferView &));

    MOCK_METHOD3(
        compare_and_set,
        outcome::result<bool>(const common::BufferView &,
                              const std::optional<common::BufferView> &,
                              common::Buffer));

    MOCK_METHOD1(get,
                 outcome::result<common::Buffer>(const common::BufferView &));
  };

}  // namespace kagome::offchain

#endif /* KAGOME_OFFCHAIN_PERSISTENT_STORAGE_MOCK_HPP */
