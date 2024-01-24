/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/babe/types/babe_configuration.hpp"
#include "primitives/block_data.hpp"

namespace kagome::consensus::babe {

  /// Keeps actual babe configuration
  class BabeConfigRepository {
   public:
    virtual ~BabeConfigRepository() = default;

    /// Returns the actual babe configuration
    /// @return the actual babe configuration
    virtual outcome::result<std::shared_ptr<const BabeConfiguration>> config(
        const primitives::BlockInfo &parent_info,
        EpochNumber epoch_number) const = 0;

    virtual void warp(const primitives::BlockInfo &block) = 0;
  };

}  // namespace kagome::consensus::babe
