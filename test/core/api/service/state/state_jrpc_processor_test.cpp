/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/state/state_jrpc_processor.hpp"

#include <gtest/gtest.h>

#include <unordered_map>

#include "mock/core/api/jrpc/jrpc_server_mock.hpp"
#include "mock/core/api/service/state/state_api_mock.hpp"
#include "testutil/literals.hpp"

using kagome::api::JRpcServer;
using kagome::api::JRpcServerMock;
using kagome::api::StateApiMock;
using kagome::api::state::StateJrpcProcessor;
using kagome::common::Buffer;
using testing::_;

class StateJrpcProcessorTest : public testing::Test {
 public:
  enum struct CallType {
    kCallType_GetRuntimeVersion = 0,
    kCallType_GetStorage,
    kCallType_StorageSubscribe,
    kCallType_StorageUnsubscribe,
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
    EXPECT_CALL(*server, registerHandler("state_getRuntimeVersion", _))
        .WillOnce(testing::Invoke([&](auto &name, auto &&f) {
          call_contexts_.emplace(
              std::make_pair(CallType::kCallType_GetRuntimeVersion,
                             CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("state_getStorage", _))
        .WillOnce(testing::Invoke([&](auto &name, auto &&f) {
          call_contexts_.emplace(std::make_pair(CallType::kCallType_GetStorage,
                                                CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("state_subscribeStorage", _))
        .WillOnce(testing::Invoke([&](auto &name, auto &&f) {
          call_contexts_.emplace(std::make_pair(
              CallType::kCallType_StorageSubscribe, CallContext{.handler = f}));
        }));
    EXPECT_CALL(*server, registerHandler("state_unsubscribeStorage", _))
        .WillOnce(testing::Invoke([&](auto &name, auto &&f) {
          call_contexts_.emplace(std::make_pair(
              CallType::kCallType_StorageUnsubscribe, CallContext{.handler = f}));
        }));
    processor.registerHandlers();
  }

  template <typename... Args>
  auto execute(CallType method, Args &&... args) {
    return call_contexts_[method].handler(std::forward<Args>(args)...);
  }

  std::shared_ptr<StateApiMock> state_api = std::make_shared<StateApiMock>();
  std::shared_ptr<JRpcServerMock> server = std::make_shared<JRpcServerMock>();
  StateJrpcProcessor processor{server, state_api};
};

/**
 * @given a request of state_getStorage with a valid param
 * @when processing it
 * @then the request is successfully processed and the response is valid
 */
TEST_F(StateJrpcProcessorTest, ProcessRequest) {
  EXPECT_CALL(*state_api, getStorage(Buffer::fromHex("01234567").value()))
      .WillOnce(testing::Return(Buffer::fromHex("ABCDEF").value()));

  registerHandlers();

  jsonrpc::Request::Parameters params{"0x01234567"};
  auto result = execute(CallType::kCallType_GetStorage, params).AsArray();
  std::vector<uint8_t> vec_result(result.size());
  std::transform(
      result.begin(), result.end(), vec_result.begin(), [](jsonrpc::Value &v) {
        return v.AsInteger32();
      });
  std::vector<uint8_t> expected_result{171, 205, 239};
  ASSERT_THAT(expected_result, testing::ContainerEq(vec_result));
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

  registerHandlers();

  jsonrpc::Request::Parameters params{"0x01234567",
                                      "0x" + ("010203"_hash256).toHex()};
  auto result = execute(CallType::kCallType_GetStorage, params).AsArray();
  std::vector<uint8_t> vec_result(result.size());
  std::transform(
      result.begin(), result.end(), vec_result.begin(), [](jsonrpc::Value &v) {
        return v.AsInteger32();
      });
  std::vector<uint8_t> expected_result{171, 205, 239};
  ASSERT_THAT(expected_result, testing::ContainerEq(vec_result));
}

/**
 * @given a request of state_getStorage with invalid params
 * @when processing it
 * @then InvalidParametersFault exception is thrown
 */

TEST_F(StateJrpcProcessorTest, InvalidParams) {
  registerHandlers();

  jsonrpc::Request::Parameters params;
  params.push_back(jsonrpc::Value{0});
  params.push_back(0);
  ASSERT_THROW(execute(CallType::kCallType_GetStorage, params).AsArray(),
               jsonrpc::InvalidParametersFault);
}

/**
 * @given a request of state_getRuntimeVersion with a valid param
 * @when processing it
 * @then the request is successfully processed and the response is valid
 */
TEST_F(StateJrpcProcessorTest, ProcessGetVersionRequest) {
  kagome::primitives::Version test_version{.spec_name = "dummy_sn",
                                           .impl_name = "dummy_in",
                                           .authoring_version = 0x101,
                                           .spec_version = 0x111,
                                           .impl_version = 0x202};

  boost::optional<kagome::primitives::BlockHash> hash = boost::none;
  EXPECT_CALL(*state_api, getRuntimeVersion(hash))
      .WillOnce(testing::Return(test_version));

  registerHandlers();

  jsonrpc::Request::Parameters params{};
  auto result =
      execute(CallType::kCallType_GetRuntimeVersion, params).AsStruct();

  ASSERT_EQ(result["authoringVersion"].AsInteger64(),
            test_version.authoring_version);
  ASSERT_EQ(result["specName"].AsString(), test_version.spec_name);
  ASSERT_EQ(result["implName"].AsString(), test_version.impl_name);
  ASSERT_EQ(result["specVersion"].AsInteger64(), test_version.spec_version);
  ASSERT_EQ(result["implVersion"].AsInteger64(), test_version.impl_version);
}

/**
 * @given a request of state_subscribeStorage with a valid param
 * @when processing it
 * @then the request is successfully processed and the response is valid
 */
TEST_F(StateJrpcProcessorTest, ProcessSubscribeStorage) {
  uint32_t val = 10;
  std::vector<Buffer> keys;
  keys.emplace_back(kagome::common::unhexWith0x("0x1011").value());
  keys.emplace_back(kagome::common::unhexWith0x("0x2002").value());

  EXPECT_CALL(*state_api, subscribeStorage(keys))
      .WillOnce(testing::Return(val));

  registerHandlers();

  jsonrpc::Value::Array data;
  data.emplace_back("0x1011");
  data.emplace_back("0x2002");

  jsonrpc::Request::Parameters params{data};
  auto result =
      execute(CallType::kCallType_StorageSubscribe, params).AsInteger32();

  ASSERT_EQ(result, val);
}

/**
 * @given a request of state_unsubscribeStorage with a valid param
 * @when processing it
 * @then the request is successfully processed and the response is valid
 */
TEST_F(StateJrpcProcessorTest, ProcessUnsubscribeStorage) {
  std::vector<uint32_t> subscription_ids = { 10 };
  EXPECT_CALL(*state_api, unsubscribeStorage(subscription_ids))
      .WillOnce(testing::Return(outcome::success()));

  registerHandlers();

  jsonrpc::Value::Array data;
  data.emplace_back(10);

  jsonrpc::Request::Parameters params{data};
  execute(CallType::kCallType_StorageUnsubscribe, params);
}
