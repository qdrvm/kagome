/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TICKER_HPP
#define KAGOME_TICKER_HPP

#include <functional>
#include <system_error>

namespace kagome::clock {
  /**
   * Interface for asynchronous ticker
   */
  struct Ticker {
    virtual ~Ticker() = default;

    /**
     * start ticker
     */
    virtual void start() = 0;

    /**
     * cancel ticker
     */
    virtual void stop() = 0;

    /**
     * Wait for the ticker interval to last
     * @param h - handler, which is called, when the ticker interval lasts, or error
     * happens
     */
    virtual void asyncCallRepeatedly(
        std::function<void(const std::error_code &)> h) = 0;
  };
}  // namespace kagome::clock

#endif  // KAGOME_TICKER_HPP
