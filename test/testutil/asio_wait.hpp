/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>

#include <latch>
#include <memory>

namespace testutil {

  /**
   * Wait for all queued tasks.
   */
  void wait(boost::asio::io_context &io) {
    auto latch = std::make_shared<std::latch>(2);
    io.post([latch] { latch->arrive_and_wait(); });
    latch->arrive_and_wait();
  }

}  // namespace testutil
