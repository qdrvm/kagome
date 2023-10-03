/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/types.hpp"
#include "primitives/block_header.hpp"

namespace kagome::consensus {

  enum class ValidatorStatus {
    NonValidator = 0,
    InactiveValidator,
    Validator,
    SingleValidator,
  };

  /// Consensus responsible for choice slot leaders, block production, etc.
  class ProductionConsensus {
   public:
    ProductionConsensus() = default;
    virtual ~ProductionConsensus() = default;

    ProductionConsensus(ProductionConsensus &&) noexcept = delete;
    ProductionConsensus(const ProductionConsensus &) = delete;
    ProductionConsensus &operator=(ProductionConsensus &&) noexcept = delete;
    ProductionConsensus &operator=(const ProductionConsensus &) = delete;

    virtual ValidatorStatus getValidatorStatus(
        const primitives::BlockInfo &parent_info,
        EpochNumber epoch_number) const = 0;

    virtual std::tuple<Duration, EpochLength> getTimings() const = 0;

    virtual outcome::result<SlotNumber> getSlot(
        const primitives::BlockHeader &header) const = 0;

    virtual outcome::result<void> processSlot(
        SlotNumber slot, const primitives::BlockInfo &parent) = 0;
  };

}  // namespace kagome::consensus
