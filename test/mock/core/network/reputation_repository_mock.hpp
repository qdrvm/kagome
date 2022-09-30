/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_REPUTATIONREPOSITORYMOCK
#define KAGOME_NETWORK_REPUTATIONREPOSITORYMOCK

#include <gmock/gmock.h>

#include "network/reputation_repository.hpp"

namespace kagome::network {

  class ReputationRepositoryMock : public ReputationRepository {
   public:
    MOCK_METHOD(Reputation, reputation, (const PeerId &), (const override));

    MOCK_METHOD(Reputation,
                change,
                (const PeerId &, ReputationChange),
                (override));

    MOCK_METHOD(Reputation,
                changeForATime,
                (const PeerId &, ReputationChange, std::chrono::seconds),
                (override));
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_REPUTATIONREPOSITORYMOCK
