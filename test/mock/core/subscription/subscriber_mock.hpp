/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_SUBSCRIBER_MOCK_HPP
#define KAGOME_TEST_MOCK_SUBSCRIBER_MOCK_HPP

namespace kagome::subscription {

  class SubscriptionTargetMock final {
   public:
    MOCK_METHOD2(test_call,
    void(std::string_view data_1,
    int32_t data_2));
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TEST_MOCK_SUBSCRIBER_MOCK_HPP
