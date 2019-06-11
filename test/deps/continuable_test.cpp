/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

// disable exceptions
#define CONTINUABLE_WITH_NO_EXCEPTIONS

// use std::error_code instead of std::error_condition
#define CONTINUABLE_WITH_CUSTOM_ERROR_TYPE std::error_code

#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <continuable/continuable-testing.hpp>
#include <continuable/continuable.hpp>

using std::chrono_literals::operator""ms;
using std::string_literals::operator""s;

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
