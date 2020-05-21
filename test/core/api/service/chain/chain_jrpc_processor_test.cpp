/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/chain/chain_jrpc_processor.hpp"

#include <gtest/gtest.h>

#include "mock/core/api/jrpc/jrpc_server_mock.hpp"
#include "mock/core/api/service/chain/chain_api_mock.hpp"
#include "testutil/literals.hpp"

using kagome::api::ChainApiMock;
using kagome::api::JRpcServer;
using kagome::api::JRpcServerMock;
using kagome::api::chain::ChainJrpcProcessor;
using kagome::common::Buffer;
using testing::_;
using testing::Return;

class ChainJrpcProcessorTest : public testing::Test {
 public:
  void SetUp() override {}

  auto registerHandlers() {
    JRpcServer::Method action;
    EXPECT_CALL(*server, registerHandler("chain_getBlockHash", _))
        .WillOnce(
            testing::Invoke([&action](auto &name, auto &&f) { action = f; }));
    processor.registerHandlers();
    return action;
  }

  std::shared_ptr<ChainApiMock> chain_api = std::make_shared<ChainApiMock>();
  std::shared_ptr<JRpcServerMock> server = std::make_shared<JRpcServerMock>();
  ChainJrpcProcessor processor{server, chain_api};
};

/**
 * @given a request of chain_getBlockHash with a valid param
 * @when process request
 * @then the request is successfully processed and the response is valid
 */
//TEST_F(ChainJrpcProcessorTest, ProcessRequest) {
//  const auto hash1 =
//      "4fee9b1803132954978652e4d73d4ec5b0dffae3832449cd5e4e4081d539aa22"_hash256;
//
//  EXPECT_CALL(*chain_api, getBlockHash(50)).WillOnce(Return(hash1));
//
//  auto action = registerHandlers();
//
//  jsonrpc::Request::Parameters params{50};
//  auto result = action(params).AsArray();
//  std::vector<uint8_t> vec_result(result.size());
//  std::transform(
//      result.begin(), result.end(), vec_result.begin(), [](jsonrpc::Value &v) {
//        return v.AsInteger32();
//      });
//  std::vector<uint8_t> expected_result{171, 205, 239};
//  ASSERT_THAT(expected_result, testing::ContainerEq(vec_result));
//}
//
///**
// * @given a request of state_getStorage with two valid params
// * @when processing it
// * @then the request is successfully processed and the response is valid
// */
//TEST_F(ChainJrpcProcessorTest, ProcessAnotherRequest) {
//  EXPECT_CALL(*state_api,
//              getStorage(Buffer::fromHex("01234567").value(), "010203"_hash256))
//      .WillOnce(testing::Return(Buffer::fromHex("ABCDEF").value()));
//
//  auto action = registerHandlers();
//
//  jsonrpc::Request::Parameters params{"0x01234567",
//                                      "0x" + ("010203"_hash256).toHex()};
//  auto result = action(params).AsArray();
//  std::vector<uint8_t> vec_result(result.size());
//  std::transform(
//      result.begin(), result.end(), vec_result.begin(), [](jsonrpc::Value &v) {
//        return v.AsInteger32();
//      });
//  std::vector<uint8_t> expected_result{171, 205, 239};
//  ASSERT_THAT(expected_result, testing::ContainerEq(vec_result));
//}
//
///**
// * @given a request of state_getStorage with invalid params
// * @when processing it
// * @then InvalidParametersFault exception is thrown
// */
//
//TEST_F(ChainJrpcProcessorTest, InvalidParams) {
//  auto action = registerHandlers();
//
//  jsonrpc::Request::Parameters params;
//  params.push_back(jsonrpc::Value{0});
//  params.push_back(0);
//  ASSERT_THROW(action(params).AsArray(), jsonrpc::InvalidParametersFault);
//}
