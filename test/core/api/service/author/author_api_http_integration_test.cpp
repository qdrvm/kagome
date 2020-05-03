/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "api/service/author/author_jrpc_processor.hpp"
#include "api/service/api_service.hpp"
#include "api/transport/impl/http/http_session.hpp"
#include "api/transport/impl/http/http_listener_impl.hpp"
#include "common/blob.hpp"
#include "core/api/client/http_client.hpp"
#include "mock/core/api/service/author/author_api_mock.hpp"
#include "primitives/extrinsic.hpp"

using namespace kagome::api;
using namespace kagome::runtime;

using kagome::api::ApiService;
using kagome::api::HttpListenerImpl;
using kagome::common::Hash256;
using kagome::primitives::Extrinsic;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;

class ESSIntegrationTest : public ::testing::Test {
  template <class T>
  using sptr = std::shared_ptr<T>;

 protected:
  using Endpoint = boost::asio::ip::tcp::endpoint;
  using Context = boost::asio::io_context;
  using Socket = boost::asio::ip::tcp::socket;
  using Timer = boost::asio::steady_timer;
  using Streambuf = boost::asio::streambuf;
  using Duration = boost::asio::steady_timer::duration;
  using ErrorCode = boost::system::error_code;

  void SetUp() override {
    extrinsic.data.put("hello world");
    hash.fill(1);
  }

  std::shared_ptr<Context> main_context = std::make_shared<Context>(1);
  std::shared_ptr<Context> client_context = std::make_shared<Context>(1);

  Endpoint endpoint = {boost::asio::ip::address::from_string("127.0.0.1"),
                       12349};
  HttpSession::Configuration http_config{};

  sptr<Listener> listener = std::make_shared<HttpListenerImpl>(
	  *main_context, HttpListenerImpl::Configuration{endpoint}, http_config);

  sptr<AuthorApiMock> api = std::make_shared<AuthorApiMock>();

  sptr<JRpcServer> server = std::make_shared<JRpcServerImpl>();

  std::vector<std::shared_ptr<JRpcProcessor>> processors{
      std::make_shared<author::AuthorJRpcProcessor>(server, api)};
  sptr<ApiService> service =
      std::make_shared<ApiService>(std::vector<std::shared_ptr<Listener>>({listener}), server, processors);

  Extrinsic extrinsic{};
  const std::string request =
      R"({"jsonrpc":"2.0","method":"author_submitExtrinsic","id":0,"params":["0x2c68656c6c6f20776f726c64"]})"
      + std::string("\n");
  Hash256 hash{};
};

/**
 * @given extrinsic submission service
 * configured with real listener and mock api, and simple api client
 * @when a valid request is submitted by client
 * @then server receives request, processes it and sends response,
 * client receives response, which matches expectation
 */
TEST_F(ESSIntegrationTest, ProcessSingleClientSuccess) {
  EXPECT_CALL(*api, submitExtrinsic(extrinsic)).WillOnce(Return(hash));

  const std::string response =
      R"({"jsonrpc":"2.0","id":0,"result":[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]})";

  const Duration timeout_duration = std::chrono::milliseconds(200);

  std::shared_ptr<test::HttpClient> client;

  ASSERT_NO_THROW(service->start());

  client = std::make_shared<test::HttpClient>(*client_context);

  std::thread client_thread([this, client, &response]() {
    ASSERT_TRUE(client->connect(endpoint));
    client->query(request, [&response](outcome::result<std::string> res) {
      ASSERT_TRUE(res);
      ASSERT_EQ(res.value(), response);
    });
  });

  main_context->run_for(timeout_duration);
  client_thread.join();
}

/**
 * @given extrinsic submission service
 * configured with real listener and mock api, and simple api client
 * @when a valid request is submitted by client
 * @then server receives request, processes it and sends response,
 * client receives response, which matches expectation
 * @and @when the same request is submitted again
 * client receives response, which matches expectation again
 */
TEST_F(ESSIntegrationTest, ProcessTwoRequestsSuccess) {
  EXPECT_CALL(*api, submitExtrinsic(extrinsic))
      .Times(2)
      .WillRepeatedly(Return(hash));

  const std::string response =
      R"({"jsonrpc":"2.0","id":0,"result":[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]})";

  const Duration timeout_duration = std::chrono::milliseconds(200);

  std::shared_ptr<test::HttpClient> client;

  ASSERT_NO_THROW(service->start());

  client = std::make_shared<test::HttpClient>(*client_context);

  std::thread client_thread([this, client, &response]() {
    ASSERT_TRUE(client->connect(endpoint));
    client->query(request, [&response](outcome::result<std::string> res) {
      ASSERT_TRUE(res);
      ASSERT_EQ(res.value(), response);
    });
    client->query(request, [&response](outcome::result<std::string> res) {
      ASSERT_TRUE(res);
      ASSERT_EQ(res.value(), response);
    });
  });

  main_context->run_for(timeout_duration);
  client_thread.join();
}
