/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extrinsics_submission_service/extrinsic_submission_service.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "runtime/tagged_transaction_queue.hpp"
#include "transaction_pool/transaction_pool.hpp"

using namespace kagome::service;
using namespace kagome::runtime;

//class TaggedTransactionQueueMock : public TaggedTransactionQueue {
// public:
//  MOCK_METHOD2(validate_transaction);
//
//};

class ExtrinsicSubmissionServiceTest : public ::testing::Test {
 protected:
  JsonTransport transport;
};
