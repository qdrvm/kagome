/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
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
  using EnginePtr = std::shared_ptr<SubscriptionEngine<std::string_view,
                                                       SubscriptionTargetMock,
                                                       std::string_view,
                                                       int32_t>>;
  using TargetPtr = std::shared_ptr<Subscriber<std::string_view,
                                               SubscriptionTargetMock,
                                               std::string_view,
                                               int32_t>>;

  EnginePtr engine_;
  std::string test_data = "test_123";
  std::string key = "key";

  void SetUp() override {
    engine_ = std::make_shared<SubscriptionEngine<std::string_view,
                                                  SubscriptionTargetMock,
                                                  std::string_view,
                                                  int32_t>>();
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

  auto subscriber = std::make_shared<Subscriber<std::string_view,
                                                SubscriptionTargetMock,
                                                std::string_view,
                                                int32_t>>(engine_);
  subscriber->setCallback(
      [&](auto set_id,
          auto &,
          const auto &key,
          std::string_view data_1,
          int32_t data_2) { target.test_call(data_1, data_2); });

  EXPECT_CALL(target, test_call(data_1, data_2));
  const auto id = subscriber->generateSubscriptionSetId();
  subscriber->subscribe(id, key);

  ASSERT_EQ(engine_->size(key), 1ull);
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

  auto subscriber = std::make_shared<Subscriber<std::string_view,
                                                SubscriptionTargetMock,
                                                std::string_view,
                                                int32_t>>(engine_);
  subscriber->setCallback([&](auto set_id,
                              auto &target,
                              auto &key,
                              std::string_view data_1,
                              int32_t data_2) { ASSERT_FALSE(true); });

  const auto id = subscriber->generateSubscriptionSetId();
  subscriber->subscribe(id, "100");

  ASSERT_EQ(engine_->size(key), 0ull);
  ASSERT_EQ(engine_->size("100"), 1ull);
  engine_->notify(key, data_1, data_2);
}

/**
 * @given a subscription engine
 * @when we add a subscriber and then make unsubscribe to the other key and make
 * notification
 * @then we NOT expect subscriber call
 */
TEST_F(SubscriptionEngineTest, SubUnsub) {
  SubscriptionTargetMock target;

  std::string_view data_1(test_data);
  int32_t data_2 = 105;

  auto subscriber = std::make_shared<Subscriber<std::string_view,
                                                SubscriptionTargetMock,
                                                std::string_view,
                                                int32_t>>(engine_);
  subscriber->setCallback([&](auto set_id,
                              auto &target,
                              auto &key,
                              std::string_view data_1,
                              int32_t data_2) { ASSERT_FALSE(true); });

  const auto id = subscriber->generateSubscriptionSetId();
  subscriber->subscribe(id, key);
  ASSERT_EQ(engine_->size(key), 1ull);

  subscriber->unsubscribe(5555ull, key);
  ASSERT_EQ(engine_->size(key), 1ull);

  subscriber->unsubscribe(id, key);
  ASSERT_EQ(engine_->size(key), 0ull);

  engine_->notify(key, data_1, data_2);
}

/**
 * @given a subscription engine
 * @when we add a subscriber and then delete it
 * @then we expect no subscribers in engine
 */
TEST_F(SubscriptionEngineTest, DeleteSub) {
  std::string_view data_1(test_data);
  int32_t data_2 = 105;
  {
    SubscriptionTargetMock target;
    auto subscriber = std::make_shared<Subscriber<std::string_view,
                                                  SubscriptionTargetMock,
                                                  std::string_view,
                                                  int32_t>>(engine_);
    subscriber->setCallback([&](auto set_id,
                                auto &target,
                                auto &key,
                                std::string_view data_1,
                                int32_t data_2) { ASSERT_FALSE(true); });

    const auto id = subscriber->generateSubscriptionSetId();
    subscriber->subscribe(id, key);
  }

  ASSERT_EQ(engine_->size(key), 0ull);
  engine_->notify(key, data_1, data_2);
}

/**
 * @given a subscription engine
 * @when we add several subscriber with the same key
 * @then we expect 1 subscribers in engine
 */
TEST_F(SubscriptionEngineTest, MultiSub) {
  std::string_view data_1(test_data);
  int32_t data_2 = 105;

  SubscriptionTargetMock target;
  auto subscriber = std::make_shared<Subscriber<std::string_view,
                                                SubscriptionTargetMock,
                                                std::string_view,
                                                int32_t>>(engine_);
  subscriber->setCallback(
      [&](auto set_id,
          auto &,
          auto &key,
          std::string_view data_1,
          int32_t data_2) { target.test_call(data_1, data_2); });

  EXPECT_CALL(target, test_call(data_1, data_2));

  const auto id = subscriber->generateSubscriptionSetId();
  subscriber->subscribe(id, key);
  subscriber->subscribe(id, key);
  subscriber->subscribe(id, key);
  subscriber->subscribe(id, key);

  ASSERT_EQ(engine_->size(key), 1ull);
  engine_->notify(key, data_1, data_2);
}

/**
 * @given a subscription engine
 * @when we add subscriber and become unsubscribe the other key
 * @then we expect 1 subscribers in engine
 */
TEST_F(SubscriptionEngineTest, UnsubscribeSub) {
  std::string_view data_1(test_data);
  int32_t data_2 = 105;

  SubscriptionTargetMock target;
  auto subscriber = std::make_shared<Subscriber<std::string_view,
                                                SubscriptionTargetMock,
                                                std::string_view,
                                                int32_t>>(engine_);
  subscriber->setCallback(
      [&](auto set_id,
          auto &,
          auto &key,
          std::string_view data_1,
          int32_t data_2) { target.test_call(data_1, data_2); });

  EXPECT_CALL(target, test_call(data_1, data_2));

  const auto id = subscriber->generateSubscriptionSetId();
  subscriber->subscribe(id, key);
  ASSERT_EQ(engine_->size(key), 1ull);

  subscriber->unsubscribe(100);
  ASSERT_EQ(engine_->size(key), 1ull);

  engine_->notify(key, data_1, data_2);
}

/**
 * @given a subscription engine
 * @when we add subscriber and become unsubscribe all
 * @then we expect 0 subscribers in engine
 */
TEST_F(SubscriptionEngineTest, UnsubscribeAllSub) {
  std::string_view data_1(test_data);
  int32_t data_2 = 105;

  SubscriptionTargetMock target;
  auto subscriber = std::make_shared<Subscriber<std::string_view,
                                                SubscriptionTargetMock,
                                                std::string_view,
                                                int32_t>>(engine_);
  subscriber->setCallback([&](auto set_id,
                              auto &,
                              auto &key,
                              std::string_view data_1,
                              int32_t data_2) { ASSERT_FALSE(true); });

  const auto id = subscriber->generateSubscriptionSetId();
  subscriber->subscribe(id, key);
  ASSERT_EQ(engine_->size(key), 1ull);

  subscriber->unsubscribe();
  ASSERT_EQ(engine_->size(key), 0ull);

  engine_->notify(key, data_1, data_2);
}

/**
 * @given a subscription engine
 * @when we add subscriber and become unsubscribe the stream
 * @then we expect 0 subscribers in engine
 */
TEST_F(SubscriptionEngineTest, UnsubscribeStreamSub) {
  std::string_view data_1(test_data);
  int32_t data_2 = 105;

  SubscriptionTargetMock target;
  auto subscriber = std::make_shared<Subscriber<std::string_view,
                                                SubscriptionTargetMock,
                                                std::string_view,
                                                int32_t>>(engine_);
  subscriber->setCallback([&](auto set_id,
                              auto &,
                              auto &key,
                              std::string_view data_1,
                              int32_t data_2) { ASSERT_FALSE(true); });

  const auto id = subscriber->generateSubscriptionSetId();
  subscriber->subscribe(id, key);
  ASSERT_EQ(engine_->size(key), 1ull);

  subscriber->unsubscribe(id);
  ASSERT_EQ(engine_->size(key), 0ull);

  engine_->notify(key, data_1, data_2);
}
