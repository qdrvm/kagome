/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/child_state/child_state_jrpc_processor.hpp"

#include <gtest/gtest.h>

#include <unordered_map>

#include <boost/range/adaptor/transformed.hpp>

#include "mock/core/api/jrpc/jrpc_server_mock.hpp"
#include "mock/core/api/service/child_state/child_state_api_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::api::JRpcServer;
using kagome::api::JRpcServerMock;
using kagome::api::ChildStateApi;
using kagome::api::ChildStateApiMock;
using kagome::api::child_state::ChildStateJrpcProcessor;
using kagome::common::Buffer;
using kagome::primitives::BlockHash;
using testing::_;

class ChildStateJrpcProcessorTest : public testing::Test {
 public:
  enum struct CallType {
    kCallType_GetKeys,
    kCallType_GetKeysPaged,
    kCallType_GetStorage,
    kCallType_GetStorageHash,
    kCallType_GetStorageSize
  };

 private:
  struct CallContext {
    JRpcServer::Method handler;
  };
  std::unordered_map<CallType, CallContext> call_contexts_;

 public:
  void SetUp() override {}

  void registerHandlers() {
    call_contexts_.clear();
    EXPECT_CALL(*server, registerHandler("childstate_getKeys", _))
        .WillOnce(testing::Invoke([&](auto &name, auto &&f) {
          call_contexts_.emplace(std::make_pair(
              CallType::kCallType_GetKeys, CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("childstate_getKeysPaged", _))
        .WillOnce(testing::Invoke([&](auto &name, auto &&f) {
          call_contexts_.emplace(std::make_pair(
              CallType::kCallType_GetKeysPaged, CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("childstate_getStorage", _))
        .WillOnce(testing::Invoke([&](auto &name, auto &&f) {
          call_contexts_.emplace(std::make_pair(CallType::kCallType_GetStorage,
                                                CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("childstate_getStorageHash", _))
        .WillOnce(testing::Invoke([&](auto &name, auto &&f) {
          call_contexts_.emplace(std::make_pair(CallType::kCallType_GetStorageHash,
                                                CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("childstate_getStorageSize", _))
        .WillOnce(testing::Invoke([&](auto &name, auto &&f) {
          call_contexts_.emplace(std::make_pair(CallType::kCallType_GetStorageSize,
                                                CallContext{.handler = f}));
        }));
    processor.registerHandlers();
  }

  template <typename... Args>
  auto execute(CallType method, Args &&...args) {
    return call_contexts_[method].handler(std::forward<Args>(args)...);
  }

  std::shared_ptr<ChildStateApiMock> child_state_api = std::make_shared<ChildStateApiMock>();
  std::shared_ptr<JRpcServerMock> server = std::make_shared<JRpcServerMock>();
  ChildStateJrpcProcessor processor{server, child_state_api};
};

/**
 * @given a request of childstate_getStorage with 2 valid params
 * @when processing it
 * @then the request is successfully processed and the response is valid
 */
TEST_F(ChildStateJrpcProcessorTest, ProcessRequest) {
  auto expected_result = "ABCDEF"_hex2buf;

  EXPECT_CALL(*child_state_api, getStorage("01234567"_hex2buf, "010203"_hex2buf, std::optional<BlockHash>{std::nullopt}))
      .WillOnce(testing::Return(expected_result));

  registerHandlers();

  jsonrpc::Request::Parameters params{"0x01234567"};
  auto result_hex = execute(CallType::kCallType_GetStorage, params).AsString();
  EXPECT_OUTCOME_TRUE(result_vec, kagome::common::unhexWith0x(result_hex));
  ASSERT_EQ(expected_result.asVector(), result_vec);
}

/**
 * @given a request of childstate_getStorage with 3 valid params
 * @when processing it
 * @then the request is successfully processed and the response is valid
 */
TEST_F(ChildStateJrpcProcessorTest, ProcessAnotherRequest) {
  auto expected_result = "ABCDEF"_hex2buf;

  EXPECT_CALL(*child_state_api, getStorage("01234567"_hex2buf, "010203"_hex2buf, std::optional<BlockHash>{"111213"_hash256}))
      .WillOnce(testing::Return(expected_result));

  registerHandlers();

  jsonrpc::Request::Parameters params{"0x01234567",
                                      "0x" + ("010203"_hash256).toHex()};
  auto result_hex = execute(CallType::kCallType_GetStorage, params).AsString();
  EXPECT_OUTCOME_TRUE(result_vec, kagome::common::unhexWith0x(result_hex));
  ASSERT_EQ(expected_result.asVector(), result_vec);
}

/**
 * @given a request of childstateGetStorage with invalid params
 * @when processing it
 * @then InvalidParametersFault exception is thrown
 */

TEST_F(ChildStateJrpcProcessorTest, InvalidParams) {
  registerHandlers();

  jsonrpc::Request::Parameters params;
  params.push_back(jsonrpc::Value{0});
  params.push_back(0);
  ASSERT_THROW(execute(CallType::kCallType_GetStorage, params).AsArray(),
               jsonrpc::InvalidParametersFault);
}
