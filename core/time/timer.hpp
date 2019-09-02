/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_COMMON_TIMER_HPP
#define KAGOME_CORE_COMMON_TIMER_HPP

#include <chrono>
#include <functional>
#include <system_error>

#include <outcome/outcome.hpp>

namespace kagome::time {

  /**
   * @brief Generic timer.
   * @tparam Clock clock type
   */
  template <typename Clock>
  struct Timer {
    virtual ~Timer() = default;

    using Duration = std::chrono::duration<Clock>;
    using TimePoint = std::chrono::duration<Clock>;

    /**
     * @brief Asynchronously wait on timer.
     * @param cb fired when timer expired or cancelled.
     */
    virtual void asyncWait(
        std::function<void(const std::error_code &ec)> cb) = 0;

    /**
     * @brief Cancel the timer
     * @returns error if any
     */
    virtual outcome::result<void> cancel() = 0;

    /**
     * @brief Getter for expiration time point for this timer.
     */
    virtual TimePoint expiresAt() const = 0;

    /**
     * @brief This function sets the expiry time. Any pending asynchronous wait
     operations will be cancelled. The handler for each cancelled operation will
     be invoked with the boost::asio::error::operation_aborted error code.
     * @param point time point
     * @return error if any or The number of asynchronous operations that were
     cancelled.
     */
    virtual outcome::result<size_t> expiresAt(TimePoint point) = 0;

    /**
     * @brief Get the timer's expiry time relative to now.
     */
    virtual Duration expiresFromNow() const = 0;

    /**
     * @brief This function sets the expiry time. Any pending asynchronous wait
     * operations will be cancelled. The handler for each cancelled operation
     * will be invoked with the boost::asio::error::operation_aborted error
     * code.
     * @param duration The expiry time to be used for the timer.
     * @return error if any or the number of asynchronous operations that were
     * cancelled.
     */
    virtual outcome::result<size_t> expiresFromNow(Duration duration) = 0;
  };

  using SystemTimer = Timer<std::chrono::system_clock>;

  using SteadyTimer = Timer<std::chrono::steady_clock>;

}  // namespace kagome::time

#endif  // KAGOME_CORE_COMMON_TIMER_HPP
