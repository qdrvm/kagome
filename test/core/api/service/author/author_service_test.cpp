/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/api_service.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "api/service/author/author_jrpc_processor.hpp"
#include "application/impl/app_state_manager_impl.hpp"
#include "common/blob.hpp"
#include "mock/core/api/service/author/author_api_mock.hpp"
#include "mock/core/api/transport/listener_mock.hpp"
#include "mock/core/api/transport/session_mock.hpp"
#include "primitives/extrinsic.hpp"

using namespace kagome::api;
using namespace kagome::api::author;

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::Extrinsic;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;

namespace kagome::api {
  enum class DummyError { ERROR = 1 };
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome::api, DummyError, e) {
  using kagome::api::DummyError;
  switch (e) {
    case DummyError::ERROR:
      return "dummy error";
  }
}

class AuthorServiceTest : public ::testing::Test {
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
  std::vector<sptr<Listener>> listeners = {listener};

  sptr<AuthorApiMock> api = std::make_shared<AuthorApiMock>();

  sptr<AppStateManager> app_state_manager =
      std::make_shared<kagome::application::AppStateManagerImpl>();

  std::shared_ptr<kagome::api::RpcContext> context =
      std::make_shared<kagome::api::RpcContext>();

  kagome::api::RpcThreadPool::Configuration config = {1, 1};

  sptr<kagome::api::RpcThreadPool> thread_pool =
      std::make_shared<kagome::api::RpcThreadPool>(context, config);

  sptr<JRpcServer> server = std::make_shared<JRpcServerImpl>();

  std::vector<std::shared_ptr<JRpcProcessor>> processors{
      std::make_shared<AuthorJRpcProcessor>(server, api)};
  sptr<ApiService> service = std::make_shared<ApiService>(
      app_state_manager, thread_pool, listeners, server, processors);

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
TEST_F(AuthorServiceTest, StartSuccess) {
  ASSERT_NO_THROW(service->start());
}

/**
 * @given extrinsic submission service configured with mock transport and mock
 * api
 * @when a valid request is submitted
 * @then request is successfully parsed and response matches expectation
 */
TEST_F(AuthorServiceTest, RequestSuccess) {
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
TEST_F(AuthorServiceTest, RequestFail) {
  EXPECT_CALL(*api, submitExtrinsic(extrinsic))
      .WillOnce(Return(outcome::failure(DummyError::ERROR)));
  std::string_view response =
      R"({"jsonrpc":"2.0","id":0,"error":{"code":0,"message":"dummy error"}})";
  EXPECT_CALL(*session, respond(response)).Times(1);
  ASSERT_NO_THROW(service->start());
  ASSERT_NO_THROW(session->processRequest(request, session));
}
