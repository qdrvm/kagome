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
  ASSERT_TRUE(r);

  r = Error {"Flibbity-jibbit"};
  ASSERT_FALSE(r);
}

/**
 * @given a Result object
 * @when trying to unwrap it to a value or an error object, which was contaned in the Result
 * @then an optional is retrieved, which contains a corresponding object or nullopt
 */
TEST(Result, Unwrap) {
  Result<int, std::string> r = Value {4};
  ASSERT_TRUE(r.tryGetValue());
  ASSERT_EQ(r.tryGetValue().value(), 4);
  ASSERT_THROW(r.tryGetError().value(), std::bad_optional_access);
  if(auto v = r.tryGetValue()) {
    ASSERT_NO_THROW(v.value());
    ASSERT_EQ(v.value(), 4);
  }

  r = Error {"Flibbity-jibbit"};
  ASSERT_TRUE(r.tryGetError());
  ASSERT_THROW(r.tryGetValue().value(), std::bad_optional_access);
  if(auto v = r.tryGetValue()) {
    FAIL() << "Must be false, as an error is stored";
  }

}
