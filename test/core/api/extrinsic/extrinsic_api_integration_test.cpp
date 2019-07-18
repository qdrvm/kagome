/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/service.hpp"

#include <chrono>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "api/transport/impl/listener_impl.hpp"
#include "api/transport/impl/session_impl.hpp"
#include "common/blob.hpp"
#include "core/api/extrinsic/simple_client.hpp"
#include "mock/api/extrinsic/extrinsic_api_mock.hpp"
#include "primitives/extrinsic.hpp"
#include "testutil/outcome.hpp"

using namespace kagome::api;
using namespace kagome::runtime;

using kagome::common::Hash256;
using kagome::primitives::Extrinsic;
using kagome::server::ListenerImpl;
using test::SimpleClient;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;

class ESSIntegrationTest : public ::testing::Test {
  template <class T>
  using sptr = std::shared_ptr<T>;

 protected:
  using Context = SimpleClient::Context;
  using ErrorCode = SimpleClient::ErrorCode;

  void SetUp() override {
    extrinsic.data.put("hello world");
    hash.fill(1);
  }

  SimpleClient::Context main_context{1};
  SimpleClient::Context client_context{1};

  SimpleClient::Endpoint endpoint = {
      boost::asio::ip::address::from_string("127.0.0.1"), 12349};
  ListenerImpl::Configuration listener_config{std::chrono::milliseconds(100)};
  sptr<ListenerImpl> listener =
      std::make_shared<ListenerImpl>(main_context, endpoint, listener_config);

  sptr<ExtrinsicApiMock> api = std::make_shared<ExtrinsicApiMock>();

  sptr<ExtrinsicApiService> service =
      std::make_shared<ExtrinsicApiService>(listener, api);

  Extrinsic extrinsic{};
  const std::string request =
      R"({"jsonrpc":"2.0","method":"author_submitExtrinsic","id":0,"params":["68656C6C6F20776F726C64"]})"
      + std::string("\n");
  Hash256 hash{};
};

/**
 * @given extrinsic submission service
 * configured with real listener and mock api, and simple tcp client
 * @when a valid request is submitted by client
 * @then server receives request, processes it and sends response,
 * client receives response, which matches expectation
 */
TEST_F(ESSIntegrationTest, ProcessSingleClientSuccess) {
  EXPECT_CALL(*api, submitExtrinsic(extrinsic)).WillOnce(Return(hash));

  const std::string response =
      R"({"jsonrpc":"2.0","id":0,"result":[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]})";

  const SimpleClient::Duration timeout_duration = std::chrono::milliseconds(200);

  ASSERT_NO_THROW(service->start());

  std::shared_ptr<SimpleClient> client;
  client = std::make_shared<SimpleClient>(client_context, timeout_duration,
                                          [client]() {
                                            client->stop();
                                            FAIL();
                                          });

  std::thread client_thread(
      [this, client, timeout_duration, response, request = this->request]() {
        auto on_error = [&](const auto &error) {
          std::cout << "error occured" << std::endl;
          client->stop();
          FAIL();
        };

        // make client and start
        auto on_read_success = [&](const ErrorCode &error, std::size_t n) {
          std::cout << "handle read" << std::endl;
          if (!error) {
            std::cout << "read success" << std::endl;
            ASSERT_EQ(client->data(), response);
          } else {
            on_error(error);
          }
        };

        auto on_write_success = [&](const ErrorCode &error, std::size_t n) {
          if (!error) {
            std::cout << "write success" << std::endl;
            client->asyncRead(on_read_success);
          } else {
            on_error(error);
          }
        };

        auto on_connect_success = [&](const ErrorCode &error) {
          if (!error) {
            std::cout << "connect success" << std::endl;
            client->asyncWrite(request, on_write_success);
          } else {
            on_error(error);
          }
        };

        client->asyncConnect(endpoint, on_connect_success);

        client->getContext().run_for(timeout_duration);
      });

  main_context.run_for(timeout_duration);
  client_thread.join();
}
