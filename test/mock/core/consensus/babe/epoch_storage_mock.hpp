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
                 outcome::result<void>(EpochIndex,
                                       const NextEpochDescriptor &));
    MOCK_CONST_METHOD1(
        getEpochDescriptor,
        outcome::result<NextEpochDescriptor>(EpochIndex epoch_number));

    MOCK_METHOD1(setLastEpoch,
                 outcome::result<void>(const LastEpochDescriptor &led));

    MOCK_CONST_METHOD0(getLastEpoch, outcome::result<LastEpochDescriptor>());
  };

}  // namespace kagome::consensus

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_BABE_EPOCH_STORAGE_MOCK_HPP
