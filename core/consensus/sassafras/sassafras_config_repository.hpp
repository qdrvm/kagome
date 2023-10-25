/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/sassafras/types/sassafras_configuration.hpp"
#include "primitives/block_data.hpp"

namespace kagome::consensus::sassafras {

  /// Keeps actual sassafras configuration
  class SassafrasConfigRepository {
   public:
    virtual ~SassafrasConfigRepository() = default;

    /// @return the duration of a slot
    virtual SlotDuration slotDuration() const = 0;

    /// @return the epoch length in slots
    virtual EpochLength epochLength() const = 0;

    /// @return the actual sassafras configuration
    virtual outcome::result<std::shared_ptr<const Epoch>> config(
        const primitives::BlockInfo &parent_info,
        EpochNumber epoch_number) const = 0;

    virtual void warp(const primitives::BlockInfo &block) = 0;
  };

}  // namespace kagome::consensus::sassafras
