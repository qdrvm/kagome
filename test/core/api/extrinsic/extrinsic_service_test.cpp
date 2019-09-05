/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/extrinsic_api_service.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "api/transport/impl/listener_impl.hpp"
#include "common/blob.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "mock/api/extrinsic/extrinsic_api_mock.hpp"
#include "mock/api/transport/listener_mock.hpp"
#include "mock/api/transport/session_mock.hpp"
#include "primitives/block_id.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/transaction.hpp"
#include "runtime/tagged_transaction_queue.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace kagome::api;
using namespace kagome::runtime;

using kagome::api::ExtrinsicApiService;
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

  sptr<ExtrinsicApiMock> api = std::make_shared<ExtrinsicApiMock>();

  sptr<ExtrinsicApiService> service =
      std::make_shared<ExtrinsicApiService>(listener, api);

  sptr<Session> session = std::make_shared<SessionMock>();

  Extrinsic extrinsic{};
  std::string request =
      R"({"jsonrpc":"2.0","method":"author_submitExtrinsic","id":0,"params":["68656C6C6F20776F726C64"]})";
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
  std::string response =
      R"({"jsonrpc":"2.0","id":0,"result":[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]})";

  // ensure received response is correct
  bool is_response_called = false;
  session->connectOnResponse([response, &is_response_called](auto r) {
    ASSERT_EQ(r, response);
    // confirm responce received
    is_response_called = true;
  });

  ASSERT_NO_THROW(service->start());

  // imitate request received
  session->processRequest(request);

  // ensure response received
  ASSERT_EQ(is_response_called, true);
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
  std::string response =
      R"({"jsonrpc":"2.0","id":0,"error":{"code":0,"message":"transaction is in invalid state"}})";

  // ensure received response is correct
  bool is_response_called = false;
  session->connectOnResponse([response, &is_response_called](auto r) {
    ASSERT_EQ(r, response);
    // confirm responce received
    is_response_called = true;
  });

  ASSERT_NO_THROW(service->start());

  // imitate request received

  ASSERT_NO_THROW(session->processRequest(request));

  // ensure response received
  ASSERT_EQ(is_response_called, true);
}
