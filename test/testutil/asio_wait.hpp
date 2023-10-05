/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>

#include <barrier>

namespace testutil {

  /**
   * Wait for all queued tasks.
   */
  void wait(boost::asio::io_context &io) {
    std::barrier barrier(2);
    io.post([&] { barrier.arrive_and_wait(); });
    barrier.arrive_and_wait();
  }

}  // namespace testutil
