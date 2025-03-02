/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/awaitable.hpp>
#include <qtils/outcome.hpp>

#define _CO_TRY(tmp, expr)              \
  ({                                    \
    auto tmp = (expr);                  \
    if (not tmp) co_return tmp.error(); \
    std::move(tmp).value();             \
  })
/**
 * Macro to get outcome result or return error.
 */
#define CO_TRY(expr) _CO_TRY(OUTCOME_UNIQUE, expr)

namespace kagome {
  /**
   * Return type for coroutine.
   */
  template <typename T>
  using Coro = boost::asio::awaitable<T>;

  /**
   * Return type for coroutine returning outcome.
   */
  template <typename T>
  using CoroOutcome = Coro<outcome::result<T>>;
}  // namespace kagome
