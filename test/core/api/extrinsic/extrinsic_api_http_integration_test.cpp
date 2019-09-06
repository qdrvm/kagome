/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/extrinsic_api_service.hpp"

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

using kagome::api::ListenerImpl;
using kagome::common::Hash256;
using kagome::primitives::Extrinsic;
using test::SimpleClient;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;

namespace {
  struct ClientTestHelper {
    virtual ~ClientTestHelper() = default;

    virtual void connectSuccess() = 0;
    virtual void writeSuccess() = 0;
    virtual void readSuccess(const std::string &response) = 0;
    virtual void errorOccured() = 0;
  };

  struct ClientHelperMock : public ClientTestHelper {
    ~ClientHelperMock() override = default;
    MOCK_METHOD0(connectSuccess, void(void));
    MOCK_METHOD0(writeSuccess, void(void));
    MOCK_METHOD1(readSuccess, void(const std::string &));
    MOCK_METHOD0(errorOccured, void(void));
  };
}  // namespace

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

  sptr<ClientHelperMock> helper = std::make_shared<ClientHelperMock>();
};

namespace {
  // inline trims string from right
  inline std::string rtrim(std::string s, std::string_view characters) {
    s.erase(std::find_if(s.rbegin(),
                         s.rend(),
                         [characters](int ch) {
                           return characters.find_first_of(ch)
                                  == std::string_view::npos;
                         })
                .base(),
            s.end());
    return s;
  }
}  // namespace

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

  const SimpleClient::Duration timeout_duration =
      std::chrono::milliseconds(200);

  std::shared_ptr<SimpleClient> client;

  ASSERT_NO_THROW(service->start());

  EXPECT_CALL(*helper, connectSuccess()).WillOnce(Return());
  EXPECT_CALL(*helper, writeSuccess()).WillOnce(Return());
  EXPECT_CALL(*helper, readSuccess(response)).WillOnce(Return());
  EXPECT_CALL(*helper, errorOccured()).Times(0);

  auto on_error = [helper = this->helper]() { helper->errorOccured(); };

  client = std::make_shared<SimpleClient>(
      client_context, timeout_duration, on_error);

  std::thread client_thread([this,
                             client,
                             timeout_duration,
                             on_error,
                             request = this->request,
                             helper = this->helper]() {
    // make client and start
    auto on_read_success = [client, on_error, helper](const ErrorCode &error,
                                                      std::size_t n) {
      if (!error) {
        auto resp = rtrim(client->data(), "\n");
        helper->readSuccess(resp);
      } else {
        client->stop();
        on_error();
      }
    };

    auto on_write_success = [client, helper, on_error, on_read_success](
                                const ErrorCode &error, std::size_t n) {
      if (!error) {
        helper->writeSuccess();
        client->asyncRead(on_read_success);
      } else {
        client->stop();
        on_error();
      }
    };

    auto on_connect_success =
        [client, helper, on_error, on_write_success, request](
            const ErrorCode &error) {
          if (!error) {
            helper->connectSuccess();
            client->asyncWrite(request, on_write_success);
          } else {
            client->stop();
            on_error();
          }
        };

    client->asyncConnect(endpoint, on_connect_success);

    client->getContext().run_for(timeout_duration);
  });

  main_context.run_for(timeout_duration);
  client_thread.join();
}
