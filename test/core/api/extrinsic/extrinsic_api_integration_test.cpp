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

  SimpleClient::Context context{2};

  SimpleClient::Endpoint endpoint = {
      boost::asio::ip::address::from_string("127.0.0.1"), 12349};
  ListenerImpl::Configuration listener_config{std::chrono::milliseconds(100)};
  sptr<ListenerImpl> listener =
      std::make_shared<ListenerImpl>(context, endpoint, listener_config);

  sptr<ExtrinsicApiMock> api = std::make_shared<ExtrinsicApiMock>();

  sptr<ExtrinsicApiService> service =
      std::make_shared<ExtrinsicApiService>(listener, api);

  Extrinsic extrinsic{};
  std::string request =
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

  std::string response =
      R"({"jsonrpc":"2.0","id":0,"result":[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]})";

  SimpleClient::Duration timeout_duration = std::chrono::milliseconds(100);

  ASSERT_NO_THROW(service->start());

  std::thread client_thread([&]() {
    SimpleClient client(context, timeout_duration, [&]() {
      client.stop();
      FAIL();
    });

    auto on_error = [&](const auto &error) {
      client.stop();
      std::cout << error.message() << std::endl;
      FAIL();
    };

    // make client and start
    auto on_read_success = [&](const ErrorCode &error, std::size_t n) {
      if (!error) {
        ASSERT_EQ(client.data(), response);
      } else {
        on_error(error);
      }
    };

    auto on_write_success = [&](const ErrorCode &error, std::size_t n) {
      if (!error) {
        client.asyncRead(on_read_success);
      } else {
        on_error(error);
      }
    };

    auto on_connect_success = [&](const ErrorCode &error) {
      if (!error) {
        client.asyncWrite(request, on_write_success);
      } else {
        on_error(error);
      }
    };

    client.asyncConnect(endpoint, on_connect_success);

    context.run_for(timeout_duration);
  });

  context.run_for(timeout_duration);
  client_thread.join();
}
