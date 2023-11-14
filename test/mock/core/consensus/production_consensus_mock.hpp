/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/production_consensus.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {
  struct ProductionConsensusMock : public ProductionConsensus {
    MOCK_METHOD(bool, isGenesisConsensus, (), (const, override));

    MOCK_METHOD(ValidatorStatus,
                getValidatorStatus,
                (const primitives::BlockInfo &, EpochNumber),
                (const, override));

    MOCK_METHOD(outcome::result<SlotNumber>,
                getSlot,
                (const primitives::BlockHeader &),
                (const, override));

    MOCK_METHOD(outcome::result<void>,
                processSlot,
                (SlotNumber, const primitives::BlockInfo &),
                (override));

    MOCK_METHOD(bool,
                changeEpoch,
                (EpochNumber epoch, const primitives::BlockInfo &block),
                (const, override));

    MOCK_METHOD(bool,
                checkSlotLeadership,
                (const primitives::BlockInfo &block, SlotNumber slot),
                (override));

    MOCK_METHOD(outcome::result<primitives::PreRuntime>,
                makePreDigest,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::Seal>,
                makeSeal,
                (const primitives::Block &block),
                (const, override));
  };
}  // namespace kagome::consensus
