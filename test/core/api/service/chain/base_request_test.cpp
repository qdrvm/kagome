/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "api/service/base_request.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
struct BaseRequestTest : public ::testing::Test {
  struct TestRequestInt32
      : kagome::api::details::RequestType<int32_t, int32_t> {
    outcome::result<int32_t> execute() override {
      return 0;
    }
  };

  struct TestRequestStr
      : kagome::api::details::RequestType<int32_t, std::string> {
    outcome::result<int32_t> execute() override {
      return 0;
    }
  };
};

/**
 * @given Parameters with int32_t param
 * @when we push this set to the RequestBase
 * @then we can get 0-index param of type int32_t with the correct value
 */
TEST_F(BaseRequestTest, Params_Int) {
  constexpr int32_t test_val = 55;

  jsonrpc::Request::Parameters params;
  params.emplace_back(test_val);

  TestRequestInt32 tr;
  ASSERT_OUTCOME_SUCCESS_TRY(tr.init(params));

  ASSERT_EQ(tr.getParam<0>(), test_val);
}

/**
 * @given Parameters with string param
 * @when we push this set to the RequestBase
 * @then we can get 0-index param of type string with the correct value
 */
TEST_F(BaseRequestTest, Params_Str) {
  std::string test_val = "test_data";

  jsonrpc::Request::Parameters params;
  params.emplace_back(test_val);

  TestRequestStr tr;
  ASSERT_OUTCOME_SUCCESS_TRY(tr.init(params));

  ASSERT_EQ(tr.getParam<0>(), test_val);
}

/**
 * @given Parameters with string param
 * @when we push this set to the RequestBase expected int32_t as a 0 param
 * @then we can get an InvalidParametersFault exception
 */
TEST_F(BaseRequestTest, Params_Invalid) {
  std::string test_val = "test_data";

  jsonrpc::Request::Parameters params;
  params.emplace_back(test_val);

  TestRequestInt32 tr;
  auto init = [&] { ASSERT_OUTCOME_SUCCESS_TRY(tr.init(params)); };
  EXPECT_THROW(init(), jsonrpc::InvalidParametersFault);
}
