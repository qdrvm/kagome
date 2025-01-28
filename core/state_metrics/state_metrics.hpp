/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace kagome::state_metrics {
  /**
    * @brief Interface for state metrics
    * It is responsible for updating the state metrics
    * (e.g. era points for the validator)
   */
  class StateMetrics {
   public:
    virtual ~StateMetrics() = default;
    /**
     * @brief Update the era points for the validator
     */
    virtual void updateEraPoints() = 0;
  };
}  // namespace kagome::state_metrics
