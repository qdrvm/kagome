/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

// needs to be included before boost/variant.hpp,
// don't move it down
#include "testutil/vector_printer.hpp"

#include "extrinsics_submission_service/extrinsic_submission_proxy.hpp"

#include <iostream>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "extrinsics_submission_service/error.hpp"
#include "primitives/auth_api.hpp"
#include "primitives/extrinsic.hpp"

using namespace kagome::service;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::Extrinsic;
using kagome::primitives::ExtrinsicKey;
using kagome::primitives::Metadata;
using kagome::primitives::Subscriber;
using kagome::primitives::SubscriptionId;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;

class ExtrinsicSubmissionApiMock : public ExtrinsicSubmissionApi {
 public:
  ~ExtrinsicSubmissionApiMock() override = default;

  MOCK_METHOD1(submit_extrinsic, outcome::result<Hash256>(const Extrinsic &));

  MOCK_METHOD0(pending_extrinsics, outcome::result<std::vector<Extrinsic>>());

  MOCK_METHOD1(
      remove_extrinsic,
      outcome::result<std::vector<Hash256>>(const std::vector<ExtrinsicKey> &));

  MOCK_METHOD3(watch_extrinsic,
               void(const Metadata &, const Subscriber &, const Buffer &));

  MOCK_METHOD2(unwatch_extrinsic,
               outcome::result<bool>(const std::optional<Metadata> &,
                                     const SubscriptionId &));
};

class ExtrinsicSubmissionProxyTest : public ::testing::Test {
  template <class T>
  using sptr = std::shared_ptr<T>;

 protected:
  sptr<ExtrinsicSubmissionApiMock> api =
      std::make_shared<ExtrinsicSubmissionApiMock>();

  ExtrinsicSubmissionProxy proxy{api};
  std::vector<uint8_t> bytes = {0, 1};
};

/**
 * @given extrinsic submission proxy instance configured with mock api
 * @when submit_extrinsic proxy method is called
 * @then submit_extrinsic api method call is executed
 * and result of proxy method corresponds to result of api method
 */
TEST_F(ExtrinsicSubmissionProxyTest, SubmitExtrinsicSuccess) {
  Hash256 hash{};
  hash.fill(1);
  std::vector<uint8_t> hash_as_vector{hash.begin(), hash.end()};

  EXPECT_CALL(*api, submit_extrinsic(_)).WillOnce(Return(hash));

  std::vector<uint8_t> result;
  ASSERT_NO_THROW(result = proxy.submit_extrinsic(bytes));
  ASSERT_EQ(result, hash_as_vector);
}

/**
 * @given extrinsic submission proxy instance configured with mock api
 * @when submit_extrinsic proxy method is called and mocked api returns error
 * @then submit_extrinsic proxy method throws jsonrpc::Fault exception
 */
TEST_F(ExtrinsicSubmissionProxyTest, SubmitExtrinsicFail) {
  EXPECT_CALL(*api, submit_extrinsic(_))
      .WillOnce(Return(outcome::failure(
          ExtrinsicSubmissionError::INVALID_STATE_TRANSACTION)));

  ASSERT_THROW(proxy.submit_extrinsic(bytes), jsonrpc::Fault);
}
