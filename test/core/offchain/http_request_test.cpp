/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

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

TEST_F(HttpRequestTest, SunnyDayScenario) {
  std::shared_ptr<HttpRequest> request;

  boost::asio::io_context io_context;

  std::thread([&] {
    auto work_guard = boost::asio::make_work_guard(io_context);
    io_context.run_for(5s);

    std::thread([&] {
      std::this_thread::sleep_for(5s);
      work_guard.reset();
      io_context.stop();
    }).detach();

  }).detach();


  RequestId id = 0;
  ASSERT_NO_THROW(request = std::make_shared<HttpRequest>(io_context, ++id));

  common::Buffer meta;
  EXPECT_TRUE(request->init(Method::GET, "https://www.google.com/", meta));

  {  // Add header
    auto r = request->addRequestHeader("X-Header", "ValueXHeader");
    EXPECT_TRUE(r.isSuccess());
  }
  {  // Add body
    auto r = request->writeRequestBody(common::Buffer().put("ThisIsBody"), 1ms);
    EXPECT_TRUE(r.isSuccess());
  }
  {  // Finalize
    auto r = request->writeRequestBody({}, 1ms);
    EXPECT_TRUE(r.isSuccess());
  }

  // Waiting loop..
  while (true) {
    auto status = request->status();
    std::cerr << "WAITING LOOP: " << status << std::endl;
    if (status != 0 or io_context.stopped()) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ASSERT_FALSE(io_context.stopped()) << "Expected io_context still ran";
  {  // Get status
    auto status = request->status();
    EXPECT_GE(status, 100) << "HTTP status expected";
  }
  {  // Get headers
    auto headers = request->getResponseHeaders();
    EXPECT_GT(headers.size(), 0) << "Some headers expected";
  }
  {  // Get content
    common::Buffer buffer;
    buffer.resize(1024);
    auto r = request->readResponseBody(buffer, 1ms);
    ASSERT_TRUE(r.isSuccess()) << "Expected successful reading body";
    EXPECT_GT(r.value(), 0) << "Non empty body expected";
  }
}
