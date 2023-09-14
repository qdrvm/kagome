/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_TESTUTIL_ASIO_WAIT_HPP
#define KAGOME_TEST_TESTUTIL_ASIO_WAIT_HPP

#include <boost/asio/io_context.hpp>
#include <future>

namespace testutil {
  /**
   * Wait for all queued tasks.
   */
  void wait(boost::asio::io_context &io) {
    std::promise<void> promise;
    auto future = promise.get_future();
    io.post([promise{std::make_shared<decltype(promise)>(std::move(promise))}] {
      promise->set_value();
    });
    future.get();
  }
}  // namespace testutil

#endif  // KAGOME_TEST_TESTUTIL_ASIO_WAIT_HPP
