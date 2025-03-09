/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>

#include "coro/coro.hpp"

namespace kagome {
  /**
   * Start coroutine on specified executor.
   * Prevents dangling lambda capture in `coroSpawn([capture] { ... })`.
   * `co_spawn([capture] { ... })` doesn't work
   * because lambda is destroyed after returning coroutine object.
   * `co_spawn([](args){ ... }(capture))`
   * works because arguments are stored in coroutine state.
   */
  void coroSpawn(auto executor, auto f) {
    boost::asio::co_spawn(
        executor,
        [](decltype(f) f) -> Coro<void> { co_await f(); }(std::move(f)),
        boost::asio::detached);
  }
}  // namespace kagome
