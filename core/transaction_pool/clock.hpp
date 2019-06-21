/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CLOCK_HPP
#define KAGOME_CLOCK_HPP

#include <chrono>

namespace kagome::transaction_pool {

  /**
   * An interface for a clock
   */
  class Clock {
   public:
    /**
     * Difference between two time points
     */
    using Duration = std::chrono::milliseconds;
    /**
     * A moment in time, stored in milliseconds since Unix epoch start
     */
    using TimePoint = std::chrono::milliseconds;

    virtual ~Clock() = default;

    /**
     * @return a time point representing the current time
     */
    virtual TimePoint now() const = 0;
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_CLOCK_HPP
