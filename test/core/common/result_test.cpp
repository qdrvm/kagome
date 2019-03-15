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
class NoCopy {
public:
  NoCopy() = default;
  NoCopy(const NoCopy&) = delete;
  NoCopy(NoCopy&&) = default;
  NoCopy& operator=(NoCopy&&) = default;
};

/**
 * @given a Result object
 * @when inspecting if it contains a value by casting it to a boolean
 * @then the Result is true if it holds a value or false otherwise
 */
TEST(Result, HasMethods) {
  Result<int, std::string> r = Value {4};
  ASSERT_TRUE(r.hasValue());
  ASSERT_FALSE(r.hasError());

  r = Error {"Flibbity-jibbit"};
  ASSERT_FALSE(r.hasValue());
  ASSERT_TRUE(r.hasError());

  r = Value {2};
  ASSERT_EQ(r.getValueRef(), 2);
  int& i = r.match(
      [](Value<int>& v) -> std::reference_wrapper<int> { return v.value;},
      [](Error<std::string>& e) -> std::reference_wrapper<int> { throw e.error; }
      );
  i = 4;
  ASSERT_EQ(r.getValueRef(), 4);
}

/**
 * @given a need to create a Result object
 * @when creating the object, assigning another value to it, getting the value
 * @then no unnecessary copying occured
 */
TEST(Result, NoCopy) {
  Result<NoCopy, NoCopy> r (Value<NoCopy>{});
  r = Error<NoCopy>();
  auto& r1 = r.getErrorRef();
  auto r2 = r.getError();
}

/**
 * @given a Result object
 * @when trying to unwrap it to a value or an error object, which was contained in the Result
 * @then the desired object is retrieved, or an UnwrapException is thrown
 */
TEST(Result, Unwrap) {
  Result<int, std::string> r = Value {4};
  ASSERT_EQ(r.getValue(), 4); // check if it's still there after the first unwrap
  ASSERT_THROW(r.getError(), UnwrapException);
  if(r.hasValue()) {
    ASSERT_NO_THROW(r.getValue());
    ASSERT_EQ(r.getValue(), 4);
  }

  r = Error {"Flibbity-jibbit"};
  ASSERT_THROW(r.getValue(), NoValueException);
  if(r.hasValue()) {
    FAIL() << "Must be false, as an error is stored";
  }
}
