/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/api_service.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "api/extrinsic/extrinsic_jrpc_processor.hpp"
#include "common/blob.hpp"
#include "mock/core/api/extrinsic/extrinsic_api_mock.hpp"
#include "mock/core/api/transport/listener_mock.hpp"
#include "mock/core/api/transport/session_mock.hpp"
#include "primitives/extrinsic.hpp"

using namespace kagome::api;
using namespace kagome::runtime;

using kagome::api::ApiService;
using kagome::api::ExtrinsicJRpcProcessor;
using kagome::api::Listener;
using kagome::api::ListenerMock;
using kagome::api::Session;
using kagome::api::SessionMock;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::BlockId;
using kagome::primitives::Extrinsic;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;

class ExtrinsicSubmissionServiceTest : public ::testing::Test {
  template <class T>
  using sptr = std::shared_ptr<T>;

  using NewSessionHandler = std::function<void(sptr<Session>)>;

 protected:
  void SetUp() override {
    // imitate listener start
    EXPECT_CALL(*listener, start(_))
        .WillRepeatedly(Invoke([this](auto &&on_new_session) mutable {
          listener->acceptOnce(on_new_session);
        }));
    extrinsic.data.put("hello world");
    hash.fill(1);

    // imitate new session
    EXPECT_CALL(*listener, acceptOnce(_))
        .WillOnce(
            Invoke([this](auto &&on_new_session) { on_new_session(session); }));
  }

  sptr<ListenerMock> listener = std::make_shared<ListenerMock>();
  std::vector<sptr<Listener>> listeners = { listener };

  sptr<ExtrinsicApiMock> api = std::make_shared<ExtrinsicApiMock>();

  sptr<JRpcServer> server = std::make_shared<JRpcServerImpl>();

  std::vector<std::shared_ptr<JRpcProcessor>> processors{
      std::make_shared<ExtrinsicJRpcProcessor>(server, api)};
  sptr<ApiService> service =
      std::make_shared<ApiService>(listeners, server, processors);

  sptr<SessionMock> session = std::make_shared<SessionMock>();

  Extrinsic extrinsic{};
  std::string request =
      R"({"jsonrpc":"2.0","method":"author_submitExtrinsic","id":0,"params":["0x2c68656c6c6f20776f726c64"]})";
  Hash256 hash{};
};

/**
 * @given extrinsic submission service configured with mock transport
 * @and mock api
 * @when start method is called
 * @then start method of transport is called
 */
TEST_F(ExtrinsicSubmissionServiceTest, StartSuccess) {
  ASSERT_NO_THROW(service->start());
}

/**
 * @given extrinsic submission service configured with mock transport and mock
 * api
 * @when a valid request is submitted
 * @then request is successfully parsed and response matches expectation
 */
TEST_F(ExtrinsicSubmissionServiceTest, RequestSuccess) {
  EXPECT_CALL(*api, submitExtrinsic(extrinsic)).WillOnce(Return(hash));
  std::string_view response =
      R"({"jsonrpc":"2.0","id":0,"result":[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]})";
  EXPECT_CALL(*session, respond(response)).Times(1);
  ASSERT_NO_THROW(service->start());

  // imitate request received
  session->processRequest(request, session);
}

/**
 * @given extrinsic submission service configured with mock transport and mock
 * api
 * @when a valid request is submitted, but mocked api returns error
 * @then request fails and response matches expectation
 */
TEST_F(ExtrinsicSubmissionServiceTest, RequestFail) {
  EXPECT_CALL(*api, submitExtrinsic(extrinsic))
      .WillOnce(Return(
          outcome::failure(ExtrinsicApiError::INVALID_STATE_TRANSACTION)));
  std::string_view response =
      R"({"jsonrpc":"2.0","id":0,"error":{"code":0,"message":"transaction is in invalid state"}})";
  EXPECT_CALL(*session, respond(response)).Times(1);
  ASSERT_NO_THROW(service->start());
  ASSERT_NO_THROW(session->processRequest(request, session));
}
