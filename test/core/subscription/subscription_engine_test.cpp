/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "subscription/subscriber.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mock/core/subscription/subscriber_mock.hpp"

using kagome::subscription::Subscriber;
using kagome::subscription::SubscriptionEngine;
using kagome::subscription::SubscriptionTargetMock;

class SubscriptionEngineTest : public testing::Test {
 public:
  using EnginePtr = std::shared_ptr<SubscriptionEngine<std::string_view, SubscriptionTargetMock, std::string_view, int32_t>>;
  using TargetPtr = std::shared_ptr<Subscriber<std::string_view, SubscriptionTargetMock, std::string_view, int32_t>>;

  EnginePtr engine_;
  std::string test_data = "test_123";
  std::string key = "key";

  void SetUp() override {
    engine_ = std::make_shared<SubscriptionEngine<std::string_view, SubscriptionTargetMock, std::string_view, int32_t>>();
  }
};

/**
 * @given a subscription engine
 * @when we add a subscriber and make notification
 * @then we expect subscriber call
 */
TEST_F(SubscriptionEngineTest, SubscriberRegistration) {
  SubscriptionTargetMock target;

    std::string_view data_1(test_data);
    int32_t data_2 = 105;

    auto subscriber = std::make_shared<Subscriber<std::string_view, SubscriptionTargetMock, std::string_view, int32_t>>(engine_);
    subscriber->set_callback([&](std::string_view data_1, int32_t data_2) {
      target.test_call(data_1, data_2);
    });

    EXPECT_CALL(target, test_call(data_1, data_2));

    subscriber->subscribe(key);
    engine_->notify(key, data_1, data_2);
}

/**
 * @given a subscription engine
 * @when we add a subscriber to the other key and make notification
 * @then we NOT expect subscriber call
 */
TEST_F(SubscriptionEngineTest, NegSubscriberRegistration) {
  SubscriptionTargetMock target;

  std::string_view data_1(test_data);
  int32_t data_2 = 105;

  auto subscriber = std::make_shared<Subscriber<std::string_view, SubscriptionTargetMock, std::string_view, int32_t>>(engine_);
  subscriber->set_callback([&](std::string_view data_1, int32_t data_2) {
    ASSERT_FALSE(true);
  });

  subscriber->subscribe("100");
  engine_->notify(key, data_1, data_2);
}
