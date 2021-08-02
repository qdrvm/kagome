/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <boost/asio.hpp>

#include <memory>

#include "clock/impl/ticker_impl.hpp"

using namespace std::literals::chrono_literals;

using namespace kagome::clock;

TEST(TickerTest, Reuse) {
  auto ctx = std::make_shared<boost::asio::io_context>();
  auto ticker = std::make_unique<TickerImpl>(ctx, 10ms);

  auto tic = std::chrono::high_resolution_clock::now();

  // won't start without callback
  ticker->start(10ms);
  ASSERT_FALSE(ticker->isStarted());

  unsigned count = 0;
  ticker->asyncCallRepeatedly([&count, t = ticker.get(), tic](const auto &) {
    if (count++ > 3) {
      t->stop();
      auto toc = std::chrono::high_resolution_clock::now();
      ASSERT_GE(std::chrono::duration_cast<std::chrono::nanoseconds>(toc - tic),
                5ms);
    }
  });
  ticker->start(10ms);
  ASSERT_TRUE(ticker->isStarted());

  ctx->run_for(100ms);
}
