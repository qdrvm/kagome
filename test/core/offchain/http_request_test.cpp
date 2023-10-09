/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
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

  RequestId id = 0;
  ASSERT_NO_THROW(request = std::make_shared<HttpRequest>(++id));

  common::Buffer meta;
  ASSERT_TRUE(request->init(HttpMethod::Get, "http://www.google.com/", meta))
      << request->errorMessage();

  {  // Add header
    auto r = request->addRequestHeader("X-Header", "ValueXHeader");
    ASSERT_TRUE(r.isSuccess()) << request->errorMessage();
  }
  {  // Add body
    auto r =
        request->writeRequestBody(common::Buffer().put("ThisIsBody"), 3000ms);
    ASSERT_TRUE(r.isSuccess()) << request->errorMessage();
  }
  {  // Finalize
    auto r = request->writeRequestBody({}, 3000ms);
    ASSERT_TRUE(r.isSuccess()) << request->errorMessage();
  }
  {  // Get status
    auto status = request->status();
    ASSERT_GE(status, 100) << "HTTP status expected; "
                           << request->errorMessage();
  }
  {  // Get headers
    auto headers = request->getResponseHeaders();
    ASSERT_GT(headers.size(), 0)
        << "Some headers expected; " << request->errorMessage();
  }
  {  // Get content
    common::Buffer buffer;
    buffer.resize(1024);
    auto r = request->readResponseBody(buffer, 3000ms);
    ASSERT_TRUE(r.isSuccess())
        << "Expected successful reading body; " << request->errorMessage();
    EXPECT_GT(r.value(), 0) << "Non empty body expected";
  }
}
