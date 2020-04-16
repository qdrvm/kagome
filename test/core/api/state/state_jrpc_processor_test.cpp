/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "api/state/state_jrpc_processor.hpp"

#include <gtest/gtest.h>

#include "mock/core/api/jrpc/jrpc_server_mock.hpp"
#include "mock/core/api/state/state_api_mock.hpp"
#include "testutil/literals.hpp"

using kagome::api::JRpcServer;
using kagome::api::JRpcServerMock;
using kagome::api::StateApiMock;
using kagome::api::StateJrpcProcessor;
using kagome::common::Buffer;
using testing::_;

class StateJrpcProcessorTest : public testing::Test {
 public:
  void SetUp() override {}

  auto registerHandlers() {
    JRpcServer::Method action;
    EXPECT_CALL(*server, registerHandler("state_getStorage", _))
        .WillOnce(
            testing::Invoke([&action](auto &name, auto &&f) { action = f; }));
    processor.registerHandlers();
    return action;
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

  auto action = registerHandlers();

  jsonrpc::Request::Parameters params {"01234567"};
  auto result = action(params).AsArray();
  std::vector<uint8_t> vec_result(result.size());
  std::transform(result.begin(), result.end(), vec_result.begin(), [](jsonrpc::Value& v) {
    return v.AsInteger32();
  });
  std::vector<uint8_t> expected_result {171,205,239};
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

  auto action = registerHandlers();

  jsonrpc::Request::Parameters params {"01234567", ("010203"_hash256).toHex()};
  auto result = action(params).AsArray();
  std::vector<uint8_t> vec_result(result.size());
  std::transform(result.begin(), result.end(), vec_result.begin(), [](jsonrpc::Value& v) {
    return v.AsInteger32();
  });
  std::vector<uint8_t> expected_result {171,205,239};
  ASSERT_THAT(expected_result, testing::ContainerEq(vec_result));
}

/**
 * @given a request of state_getStorage with invalid params
 * @when processing it
 * @then InvalidParametersFault exception is thrown
 */

TEST_F(StateJrpcProcessorTest, InvalidParams) {
  auto action = registerHandlers();

  jsonrpc::Request::Parameters params;
  params.push_back(jsonrpc::Value{0});
  params.push_back(0);
  ASSERT_THROW(action(params).AsArray(), jsonrpc::InvalidParametersFault);
}
