/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/basic/continuable.hpp"
#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <continuable/continuable-testing.hpp>

auto async_get_success() {
  return cti::make_continuable<std::string>(
      [&](auto &&promise) { promise.set_value("Hello world!"); });
}

auto async_get_error() {
  return cti::make_continuable<std::string>(
      [&](auto &&promise) { promise.set_exception(std::error_code{}); });
}

TEST(Continuable, Simple) {
  ASSERT_ASYNC_RESULT(async_get_success(), "Hello world!");
  ASSERT_ASYNC_EXCEPTION_RESULT(async_get_error(), std::error_code{});
}
