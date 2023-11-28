/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>

#include "consensus/babe/types/babe_configuration.hpp"
#include "primitives/common.hpp"

namespace kagome::runtime {

  /**
   * Api to invoke runtime entries related for Babe algorithm
   */
  class BabeApi {
   public:
    virtual ~BabeApi() = default;

    /**
     * Get configuration for the babe
     * @return Babe configuration
     */
    virtual outcome::result<consensus::babe::BabeConfiguration> configuration(
        const primitives::BlockHash &block) = 0;

    /**
     * Get next config from last digest.
     */
    virtual outcome::result<consensus::babe::Epoch> next_epoch(
        const primitives::BlockHash &block) = 0;
  };

}  // namespace kagome::runtime
