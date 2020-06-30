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
  char const *dummy_sn = "dummy_sn";
  char const *dummy_in = "dummy_in";
  uint32_t const val1 = 0x101;
  uint32_t const val2 = 0x111;
  uint32_t const val3 = 0x201;

  std::optional<kagome::primitives::BlockHash> hash = std::nullopt;
  EXPECT_CALL(*state_api, getRuntimeVersion(hash))
      .WillOnce(
          testing::Return(kagome::primitives::Version{.spec_name = dummy_sn,
                                                      .impl_name = dummy_in,
                                                      .authoring_version = val1,
                                                      .spec_version = val3,
                                                      .impl_version = val2}));

  registerHandlers();

  jsonrpc::Request::Parameters params{};
  auto result =
      execute(CallType::kCallType_GetRuntimeVersion, params).AsStruct();

  ASSERT_EQ(result["authoringVersion"].AsInteger64(), val1);
  ASSERT_EQ(result["specName"].AsString(), dummy_sn);
  ASSERT_EQ(result["implName"].AsString(), dummy_in);
  ASSERT_EQ(result["specVersion"].AsInteger64(), val3);
  ASSERT_EQ(result["implVersion"].AsInteger64(), val2);
}
