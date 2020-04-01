/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_BABE_EPOCH_STORAGE_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_BABE_EPOCH_STORAGE_MOCK_HPP

#include "consensus/babe/epoch_storage.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {

  class EpochStorageMock : public EpochStorage {
   public:
    MOCK_METHOD2(addEpochDescriptor,
                 void(EpochIndex epoch_number,
                      NextEpochDescriptor epoch_descriptor));
    MOCK_CONST_METHOD1(
        getEpochDescriptor,
        outcome::result<NextEpochDescriptor>(EpochIndex epoch_number));
  };

}  // namespace kagome::consensus

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_BABE_EPOCH_STORAGE_MOCK_HPP
