/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

// needs to be included at the top, don't move it down
#include "core/extrinsic_submission_service/extrinsic_submission_api_mock.hpp"

#include "extrinsics_submission_service/extrinsic_submission_service.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "common/blob.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "primitives/block_id.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/transaction.hpp"
#include "runtime/tagged_transaction_queue.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "transaction_pool/impl/transaction_pool_impl.hpp"
#include "transaction_pool/transaction_pool.hpp"

using namespace kagome::service;
using namespace kagome::runtime;

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::BlockId;
using kagome::primitives::Extrinsic;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;

class JsonTransportMock : public JsonTransport {
 public:
  ~JsonTransportMock() override = default;

  explicit JsonTransportMock(NetworkAddress address)
      : address_{std::move(address)} {}

  MOCK_METHOD0(start, outcome::result<void>());

  void stop() override {}

  void doRequest(std::string_view request) {
    dataReceived()(std::string(request));
  }

  MOCK_METHOD1(processResponse, void(const std::string &));

 private:
  NetworkAddress address_;
};

class ExtrinsicSubmissionServiceTest : public ::testing::Test {
  template <class T>
  using sptr = std::shared_ptr<T>;

 protected:
  void SetUp() override {
    EXPECT_CALL(*transport, start()).WillRepeatedly(Return(outcome::success()));
    extrinsic.data.put("hello world");
    hash.fill(1);
  }

  ExtrinsicSubmissionService::Configuration configuration = {
      boost::asio::ip::make_address_v4("127.0.0.1"), 1234};

  sptr<JsonTransportMock> transport =
      std::make_shared<JsonTransportMock>(configuration.address);

  sptr<ExtrinsicSubmissionApiMock> api =
      std::make_shared<ExtrinsicSubmissionApiMock>();

  sptr<ExtrinsicSubmissionService> service =
      std::make_shared<ExtrinsicSubmissionService>(configuration, transport,
                                                   api);

  Extrinsic extrinsic{};
  std::string request =
      "{\"jsonrpc\":\"2.0\",\"method\":\"author_submitExtrinsic\",\"id\":0,"
      "\"params\":[\"68656C6C6F20776F726C64\"]}";
  Hash256 hash{};
};

/**
 * @given extrinsic submission service configured with mock transport and mock
 * api
 * @when start method is called
 * @then start method of transport is called
 */
TEST_F(ExtrinsicSubmissionServiceTest, StartSuccess) {
  EXPECT_CALL(*transport, start()).WillOnce(Return(outcome::success()));
  ASSERT_EQ(service->start(), outcome::success());
}

/**
 * @given extrinsic submission service configured with mock transport and mock
 * api
 * @when a valid request is submitted
 * @then request is successfully parsed and response matches expectation
 */
TEST_F(ExtrinsicSubmissionServiceTest, RequestSuccess) {
  EXPECT_CALL(*api, submit_extrinsic(extrinsic)).WillOnce(Return(hash));
  std::string response =
      R"({"jsonrpc":"2.0","id":0,"result":[1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]})";
  EXPECT_CALL(*transport, processResponse(response)).WillOnce(Return());

  transport->doRequest(request);
}

/**
 * @given extrinsic submission service configured with mock transport and mock
 * api
 * @when a valid request is submitted, but mocked api returns error
 * @then request fails and response matches expectation
 */
TEST_F(ExtrinsicSubmissionServiceTest, RequestFail) {
  EXPECT_CALL(*api, submit_extrinsic(extrinsic))
      .WillOnce(Return(outcome::failure(
          ExtrinsicSubmissionError::INVALID_STATE_TRANSACTION)));
  std::string response =
      R"({"jsonrpc":"2.0","id":0,"error":{"code":0,"message":"transaction is in invalid state"}})";

  EXPECT_CALL(*transport, processResponse(_)).WillOnce(Return());

  transport->doRequest(request);
}
