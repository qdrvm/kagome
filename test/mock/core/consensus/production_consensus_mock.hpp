/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/production_consensus.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {
  struct ProductionConsensusMock : public ProductionConsensus {
    MOCK_METHOD(ValidatorStatus,
                getValidatorStatus,
                (const primitives::BlockInfo &, EpochNumber),
                (const, override));

    using Timings = std::tuple<Duration, EpochLength>;

    MOCK_METHOD(Timings, getTimings, (), (const, override));

    MOCK_METHOD(outcome::result<SlotNumber>,
                getSlot,
                (const primitives::BlockHeader &),
                (const, override));
    MOCK_METHOD(outcome::result<void>,
                processSlot,
                (SlotNumber, const primitives::BlockInfo &),
                (override));
  };
}  // namespace kagome::consensus
