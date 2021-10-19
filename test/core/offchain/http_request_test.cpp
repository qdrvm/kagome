/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "offchain/impl/http_request.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using namespace offchain;
using namespace std::chrono_literals;

class HttpRequestTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
    log::setLevelOfGroup("offchain", log::Level::TRACE);
  }
  void SetUp() override {}
};

TEST_F(HttpRequestTest, Init) {
  std::shared_ptr<HttpRequest> request;

  boost::asio::io_context io_context;
  RequestId id = 0;

  common::Buffer meta;

  ASSERT_NO_THROW(request = std::make_shared<HttpRequest>(io_context, ++id));

  EXPECT_TRUE(
      request->init(Method::GET, "https://google.com/PATH?QUERY#FRAG", meta));
  ASSERT_NO_THROW(request.reset());
}

TEST_F(HttpRequestTest, Resolve) {
  std::shared_ptr<HttpRequest> request;

  boost::asio::io_context io_context;
  RequestId id = 0;

  common::Buffer meta;

  ASSERT_NO_THROW(request = std::make_shared<HttpRequest>(io_context, ++id));

  EXPECT_TRUE(
      request->init(Method::GET, "https://google.com/PATH?QUERY#FRAG", meta));

  io_context.run_for(3s);
}

TEST_F(HttpRequestTest, Connect) {
  std::shared_ptr<HttpRequest> request;

  boost::asio::io_context io_context;
  RequestId id = 0;

  common::Buffer meta;

  ASSERT_NO_THROW(request = std::make_shared<HttpRequest>(io_context, ++id));

  EXPECT_TRUE(
      request->init(Method::GET, "https://google.com/PATH?QUERY#FRAG", meta));

  io_context.run_for(3s);
}

TEST_F(HttpRequestTest, SunnyDayScenario) {
  std::shared_ptr<HttpRequest> request;

  boost::asio::io_context io_context;
  RequestId id = 0;

  common::Buffer meta;

  ASSERT_NO_THROW(request = std::make_shared<HttpRequest>(io_context, ++id));

  EXPECT_TRUE(
      request->init(Method::POST,
                    "http://127.0.0.1:9933/path/to/file?key=value#anchor",
                    meta));

  {  // Add header
    auto r = request->addRequestHeader("X-Header", "ValueXHeader");
    EXPECT_TRUE(r.isSuccess());
  }
  {  // Add body
    auto r = request->writeRequestBody(common::Buffer().put("ThisIsBody"), {});
    EXPECT_TRUE(r.isSuccess());
  }
  {  // Finalize
    auto r = request->writeRequestBody(common::Buffer(), {});
    EXPECT_TRUE(r.isSuccess());
  }

  io_context.run_for(5s);
}
