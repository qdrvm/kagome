/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>

#include <latch>

namespace testutil {

  /**
   * Wait for all queued tasks.
   */
  void wait(boost::asio::io_context &io) {
    std::latch latch(2);
    io.post([&] { latch.arrive_and_wait(); });
    latch.arrive_and_wait();
  }

}  // namespace testutil
