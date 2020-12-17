/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/author/impl/author_api_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <primitives/event_types.hpp>

#include "common/blob.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/network/extrinsic_gossiper_mock.hpp"
#include "mock/core/runtime/tagged_transaction_queue_mock.hpp"
#include "mock/core/transaction_pool/transaction_pool_mock.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/transaction.hpp"
#include "subscription/subscription_engine.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/outcome/dummy_error.hpp"
#include "testutil/primitives/mp_utils.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

using namespace kagome::api;
using namespace kagome::crypto;
using namespace kagome::transaction_pool;
using namespace kagome::runtime;

using kagome::blockchain::BlockTree;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::network::ExtrinsicGossiperMock;
using kagome::primitives::BlockId;
using kagome::primitives::BlockInfo;
using kagome::primitives::Extrinsic;
using kagome::primitives::InvalidTransaction;
using kagome::primitives::Transaction;
using kagome::primitives::TransactionSource;
using kagome::primitives::TransactionValidity;
using kagome::primitives::UnknownTransaction;
using kagome::primitives::ValidTransaction;
using kagome::primitives::events::ExtrinsicEventSubscriber;
using kagome::primitives::events::ExtrinsicEventType;
using kagome::primitives::events::ExtrinsicLifecycleEvent;
using kagome::primitives::events::ExtrinsicSubscriptionEngine;
using kagome::subscription::Subscriber;
using kagome::subscription::SubscriptionEngine;

using ::testing::_;
using ::testing::ByRef;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnRef;

using testutil::createHash256;
using testutil::DummyError;

class ExtrinsicEventReceiver {
 public:
  virtual ~ExtrinsicEventReceiver() = default;

  virtual void receive(kagome::subscription::SubscriptionSetId,
                       std::shared_ptr<kagome::api::Session>,
                       const kagome::primitives::Transaction::ObservedId &,
                       const ExtrinsicLifecycleEvent &) const = 0;
};

class ExtrinsicEventReceiverMock : public ExtrinsicEventReceiver {
 public:
  MOCK_CONST_METHOD4(receive,
                     void(kagome::subscription::SubscriptionSetId,
                          std::shared_ptr<kagome::api::Session>,
                          const kagome::primitives::Transaction::ObservedId &,
                          const ExtrinsicLifecycleEvent &));
};

struct AuthorApiTest : public ::testing::Test {
  template <class T>
  using sptr = std::shared_ptr<T>;

  sptr<HasherMock> hasher;               ///< hasher mock
  sptr<TaggedTransactionQueueMock> ttq;  ///< tagged transaction queue mock
  sptr<TransactionPoolMock> transaction_pool;  ///< transaction pool mock
  sptr<ExtrinsicGossiperMock> gossiper;        ///< gossiper mock
  sptr<AuthorApiImpl> api;                     ///< api instance
  sptr<Extrinsic> extrinsic;                   ///< extrinsic instance
  sptr<ValidTransaction> valid_transaction;    ///< valid transaction instance
  Hash256 deepest_hash;                        ///< hash of deepest leaf
  sptr<BlockInfo> deepest_leaf;                ///< deepest leaf block info
  sptr<ExtrinsicSubscriptionEngine> sub_engine;
  sptr<ExtrinsicEventSubscriber> subscriber;
  kagome::subscription::SubscriptionSetId sub_id;
  const kagome::primitives::Transaction::ObservedId ext_id = 42;
  sptr<ExtrinsicEventReceiverMock> event_receiver;

  void SetUp() override {
    sub_engine = std::make_shared<ExtrinsicSubscriptionEngine>();
    subscriber =
        std::make_unique<ExtrinsicEventSubscriber>(sub_engine, nullptr);
    event_receiver = std::make_shared<ExtrinsicEventReceiverMock>();
    sub_id = subscriber->generateSubscriptionSetId();
    subscriber->subscribe(sub_id, ext_id);
    subscriber->setCallback(
        [this](
            kagome::subscription::SubscriptionSetId set_id,
            std::shared_ptr<kagome::api::Session> session,
            const kagome::primitives::Transaction::ObservedId &id,
            const kagome::primitives::events::ExtrinsicLifecycleEvent &event) {
          event_receiver->receive(set_id, session, id, event);
        });

    hasher = std::make_shared<HasherMock>();
    ttq = std::make_shared<TaggedTransactionQueueMock>();
    transaction_pool = std::make_shared<TransactionPoolMock>();
    gossiper = std::make_shared<ExtrinsicGossiperMock>();
    api = std::make_shared<AuthorApiImpl>(
        ttq, transaction_pool, hasher, gossiper);
    extrinsic.reset(new Extrinsic{"12"_hex2buf});
    valid_transaction.reset(new ValidTransaction{1, {{2}}, {{3}}, 4, true});
    deepest_hash = createHash256({1u, 2u, 3u});
    deepest_leaf.reset(new BlockInfo{1u, deepest_hash});
  }
};

/**
 * @given configured extrinsic submission api object
 * @when submit_extrinsic is called
 * @then it is successfully completes returning expected result
 */
TEST_F(AuthorApiTest, SubmitExtrinsicSuccess) {
  TransactionValidity tv = *valid_transaction;
  gsl::span<const uint8_t> span = gsl::make_span(extrinsic->data);
  EXPECT_CALL(*hasher, blake2b_256(span)).WillOnce(Return(Hash256{}));
  EXPECT_CALL(*ttq,
              validate_transaction(TransactionSource::External, *extrinsic))
      .WillOnce(Return(tv));
  Transaction tr{ext_id,
                 *extrinsic,
                 extrinsic->data.size(),
                 Hash256{},
                 valid_transaction->priority,
                 valid_transaction->longevity,
                 valid_transaction->requires,
                 valid_transaction->provides,
                 true};
  EXPECT_CALL(*transaction_pool, submitOne(tr))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*gossiper, propagateTransactions(_)).Times(1);
  EXPECT_OUTCOME_SUCCESS(hash, api->submitExtrinsic(*extrinsic));
  ASSERT_EQ(hash.value(), Hash256{});
}

/**
 * @given configured extrinsic submission api object
 * @when submit_extrinsic is called,
 * but in process extrinsic was recognized as `Invalid`
 * @then method returns failure and extrinsic is not sent
 * to transaction pool
 */
TEST_F(AuthorApiTest, SubmitExtrinsicFail) {
  TransactionValidity tv = InvalidTransaction{1u};
  EXPECT_CALL(*ttq,
              validate_transaction(TransactionSource::External, *extrinsic))
      .WillOnce(Return(outcome::failure(DummyError::ERROR)));
  EXPECT_CALL(*hasher, blake2b_256(_)).Times(0);
  EXPECT_CALL(*transaction_pool, submitOne(_)).Times(0);
  EXPECT_CALL(*gossiper, propagateTransactions(_)).Times(0);
  EXPECT_OUTCOME_ERROR(
      res, api->submitExtrinsic(*extrinsic), DummyError::ERROR);
}

MATCHER_P(eventsAreEqual, n, "") {
  return (arg.id == n.id) and (arg.type == n.type);
}

/**
 * @given an extrinsic
 * @when submitting it through author api
 * @then it is successfully submitted, passed to the transaction pool and
 * propagated via gossiper, with corresponding events catched
 */
TEST_F(AuthorApiTest, SubmitAndWatchExtrinsicSubmitsAndWatches) {
  TransactionValidity tv = *valid_transaction;
  gsl::span<const uint8_t> span = gsl::make_span(extrinsic->data);
  EXPECT_CALL(*hasher, blake2b_256(span)).WillOnce(Return(Hash256{}));
  EXPECT_CALL(*ttq,
              validate_transaction(TransactionSource::External, *extrinsic))
      .WillOnce(Return(tv));
  Transaction tr{ext_id,
                 *extrinsic,
                 extrinsic->data.size(),
                 Hash256{},
                 valid_transaction->priority,
                 valid_transaction->longevity,
                 valid_transaction->requires,
                 valid_transaction->provides,
                 true};
  EXPECT_CALL(*transaction_pool, submitOne(tr))
      .WillOnce(testing::DoAll(
          testing::Invoke([this](auto &) {
            sub_engine->notify(ext_id, ExtrinsicLifecycleEvent::Future(ext_id));
            sub_engine->notify(ext_id, ExtrinsicLifecycleEvent::Ready(ext_id));
          }),
          Return(outcome::success())));

  EXPECT_CALL(*gossiper, propagateTransactions(_))
      .WillOnce(testing::Invoke([this](auto &) {
        sub_engine->notify(ext_id,
                           ExtrinsicLifecycleEvent::Broadcast(ext_id, {}));
      }));

  {
    testing::InSequence s;
    EXPECT_CALL(
        *event_receiver,
        receive(sub_id,
                sptr<kagome::api::Session>(),
                ext_id,
                eventsAreEqual(ExtrinsicLifecycleEvent::Future(ext_id))))
        .Times(1);
    EXPECT_CALL(*event_receiver,
                receive(sub_id,
                        sptr<kagome::api::Session>(),
                        ext_id,
                        eventsAreEqual(ExtrinsicLifecycleEvent::Ready(ext_id))))
        .Times(1);
    EXPECT_CALL(
        *event_receiver,
        receive(sub_id,
                sptr<kagome::api::Session>(),
                ext_id,
                eventsAreEqual(ExtrinsicLifecycleEvent::Broadcast(ext_id, {}))))
        .Times(1);
  }

  // throws because api service is uninitialized
  EXPECT_THROW(api->submitAndWatchExtrinsic(*extrinsic),
               jsonrpc::InternalErrorFault);
}
