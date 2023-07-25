/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORYMOCK
#define KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORYMOCK

#include "consensus/babe/babe_config_repository.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::babe {

  class BabeConfigRepositoryMock : public BabeConfigRepository {
   public:
    MOCK_METHOD(BabeDuration, slotDuration, (), (const, override));

    MOCK_METHOD(EpochLength, epochLength, (), (const, override));

    MOCK_METHOD(
        outcome::result<std::shared_ptr<const primitives::BabeConfiguration>>,
        config,
        (const primitives::BlockInfo &, EpochNumber),
        (const, override));

    MOCK_METHOD(void, warp, (const primitives::BlockInfo &), (override));
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CONSENSUS_BABE_BABECONFIGREPOSITORYMOCK
