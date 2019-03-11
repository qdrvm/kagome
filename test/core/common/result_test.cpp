/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include "common/result.hpp"

using kagome::expected::Result;
using kagome::expected::Value;
using kagome::expected::Error;
using kagome::expected::NoValueException;
using kagome::expected::UnwrapException;

/**
 * Created to detect unnecessary copying of values in Result
 */
class T {
public:
  T() { spdlog::info("T()"); }
  T(const T&) { spdlog::info("T(const T&)"); }
  T(T&&) { spdlog::info("T(T&&)"); }
};

/**
 * @given a Result object
 * @when inspecting if it contains a value by casting it to a boolean
 * @then the Result is true if it holds a value or false otherwise
 */
TEST(Result, ToBool) {
  Result<int, std::string> r = Value {4};
  ASSERT_TRUE((bool)r);

  r = Error {"Flibbity-jibbit"};
  ASSERT_FALSE((bool)r);
}

/**
 * @given a Result object
 * @when trying to unwrap it to a value or an error object, which was contaned in the Result
 * @then the desired object is retrieved, or an UnwrapException is thrown
 */
TEST(Result, Unwrap) {
  Result<int, std::string> r = Value {4};
  ASSERT_EQ(r.tryExtractValue(), 4);
  ASSERT_EQ(r.tryExtractValue(), 4); // check if it's still there after the first unwrap
  ASSERT_THROW(r.tryExtractError(), UnwrapException);
  if(r) {
    ASSERT_NO_THROW(r.tryExtractValue());
    ASSERT_EQ(r.tryExtractValue(), 4);
  }

  r = Error {"Flibbity-jibbit"};
  ASSERT_THROW(r.tryExtractValue(), NoValueException);
  if(r) {
    FAIL() << "Must be false, as an error is stored";
  }
}
