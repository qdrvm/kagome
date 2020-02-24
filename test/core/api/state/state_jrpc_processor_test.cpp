/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/state/state_jrpc_processor.hpp"

#include <gtest/gtest.h>

#include "mock/core/api/state/state_api_mock.hpp"
#include "testutil/literals.hpp"

using kagome::api::StateApiMock;
using kagome::api::StateJrpcProcessor;
using kagome::common::Buffer;

class StateJrpcProcessorTest : public testing::Test {
 public:
  void SetUp() override {
    processor.registerHandlers();
  }

  std::shared_ptr<StateApiMock> state_api = std::make_shared<StateApiMock>();
  StateJrpcProcessor processor{state_api};
};

/**
 * @given a request of state_getStorage with a valid param
 * @when processing it
 * @then the request is successfully processed and the response is valid
 */
TEST_F(StateJrpcProcessorTest, ProcessRequest) {
  EXPECT_CALL(*state_api, getStorage(Buffer::fromHex("01234567").value()))
      .WillOnce(testing::Return(Buffer::fromHex("ABCDEF").value()));

  processor.processData(
      R"({"jsonrpc":"2.0", "method": "state_getStorage", "id" : 0, "params":["01234567"]})",
      [](auto &&response) {
        ASSERT_STREQ(R"({"jsonrpc":"2.0","id":0,"result":[171,205,239]})",
                     response.data());
      });
}

/**
 * @given a request of state_getStorage with two valid params
 * @when processing it
 * @then the request is successfully processed and the response is valid
 */
TEST_F(StateJrpcProcessorTest, ProcessAnotherRequest) {
  EXPECT_CALL(*state_api,
              getStorage(Buffer::fromHex("01234567").value(), "010203"_hash256))
      .WillOnce(testing::Return(Buffer::fromHex("ABCDEF").value()));

  processor.processData(
      R"({"jsonrpc":"2.0", "method": "state_getStorage", "id" : 0, "params":["01234567", ")"
          + ("010203"_hash256).toHex() + "\"]}",
      [](auto &&response) {
        ASSERT_STREQ(R"({"jsonrpc":"2.0","id":0,"result":[171,205,239]})",
                     response.data());
      });
}

TEST_F(StateJrpcProcessorTest, InvalidParams) {
  processor.processData(
      R"({"jsonrpc":"2.0", "method": "state_getStorage", "id" : 0, "params":[0, 0]})",
      [](auto &&response) {
        ASSERT_STREQ(
            R"({"jsonrpc":"2.0","id":0,"error":{"code":-32602,"message":"Parameter 'key' must be a hex string"}})",
            response.data());
      });
}
