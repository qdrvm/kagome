/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/author/impl/author_api_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <jsonrpc-lean/fault.h>
#include <type_traits>

#include "common/blob.hpp"
#include "common/hexutil.hpp"
#include "crypto/crypto_store/crypto_store_impl.hpp"
#include "crypto/crypto_store/key_file_storage.hpp"
#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/ed25519_types.hpp"
#include "crypto/sr25519_types.hpp"
#include "mock/core/api/service/api_service_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/crypto/crypto_store_mock.hpp"
#include "mock/core/network/transactions_transmitter_mock.hpp"
#include "mock/core/runtime/session_keys_api_mock.hpp"
#include "mock/core/transaction_pool/transaction_pool_mock.hpp"
#include "primitives/event_types.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/transaction.hpp"
#include "subscription/subscription_engine.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/outcome/dummy_error.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/primitives/mp_utils.hpp"
#include "testutil/sr25519_utils.hpp"

using namespace kagome::api;
using namespace kagome::common;
using namespace kagome::crypto;
using namespace kagome::transaction_pool;
using namespace kagome::runtime;

using kagome::blockchain::BlockTreeMock;
using kagome::network::TransactionsTransmitterMock;
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
using ::testing::InSequence;
using ::testing::Return;
using ::testing::ReturnRef;

using testutil::createHash256;
using testutil::DummyError;

class ExtrinsicEventReceiver {
 public:
  virtual ~ExtrinsicEventReceiver() = default;

  virtual void receive(
      kagome::subscription::SubscriptionSetId,
      std::shared_ptr<kagome::api::Session>,
      const kagome::primitives::events::SubscribedExtrinsicId &id,
      const ExtrinsicLifecycleEvent &) const = 0;
};

class ExtrinsicEventReceiverMock : public ExtrinsicEventReceiver {
 public:
  MOCK_METHOD(void,
              receive,
              (kagome::subscription::SubscriptionSetId,
               std::shared_ptr<kagome::api::Session>,
               const kagome::primitives::events::SubscribedExtrinsicId &id,
               const ExtrinsicLifecycleEvent &),
              (const, override));
};

struct AuthorApiTest : public ::testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  template <class T>
  using sptr = std::shared_ptr<T>;

  kagome::network::Roles role;
  sptr<CryptoStoreMock> store;
  sptr<SessionKeys> keys;
  sptr<KeyFileStorage> key_store;
  Sr25519Keypair key_pair;
  sptr<SessionKeysApi> key_api;
  sptr<TransactionPoolMock> transaction_pool;
  sptr<ApiServiceMock> api_service_mock;
  sptr<AuthorApiImpl> author_api;
  sptr<Extrinsic> extrinsic;
  sptr<ValidTransaction> valid_transaction;
  Hash256 deepest_leaf_hash;
  sptr<BlockInfo> deepest_leaf;
  sptr<BlockTreeMock> block_tree;
  sptr<ExtrinsicSubscriptionEngine> sub_engine;
  sptr<ExtrinsicEventSubscriber> subscriber;
  kagome::subscription::SubscriptionSetId sub_id;
  const kagome::primitives::events::SubscribedExtrinsicId ext_id = 42;
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
            const kagome::primitives::events::SubscribedExtrinsicId &id,
            const kagome::primitives::events::ExtrinsicLifecycleEvent &event) {
          event_receiver->receive(set_id, session, id, event);
        });

    store = std::make_shared<CryptoStoreMock>();
    key_store = KeyFileStorage::createAt("test_chain_43/keystore").value();
    key_pair = generateSr25519Keypair();
    ASSERT_OUTCOME_SUCCESS_TRY(key_store->saveKeyPair(
        KEY_TYPE_BABE,
        gsl::make_span(key_pair.public_key.data(), 32),
        gsl::make_span(std::array<uint8_t, 1>({1}).begin(), 1)));
    role.flags.authority = 1;
    keys = std::make_shared<SessionKeys>(store, role);
    key_api = std::make_shared<SessionKeysApiMock>();
    transaction_pool = std::make_shared<TransactionPoolMock>();
    block_tree = std::make_shared<BlockTreeMock>();
    api_service_mock = std::make_shared<ApiServiceMock>();
    author_api = std::make_shared<AuthorApiImpl>(
        key_api, transaction_pool, store, keys, key_store, block_tree);
    author_api->setApiService(api_service_mock);
    extrinsic.reset(new Extrinsic{"12"_hex2buf});
    valid_transaction.reset(new ValidTransaction{1, {{2}}, {{3}}, 4, true});
    deepest_leaf_hash = createHash256({1u, 2u, 3u});
    deepest_leaf.reset(new BlockInfo{1u, deepest_leaf_hash});
  }
};

/**
 * @given configured extrinsic submission api object
 * @when submit_extrinsic is called
 * @then it is successfully completes returning expected result
 */
TEST_F(AuthorApiTest, SubmitExtrinsicSuccess) {
  EXPECT_CALL(*transaction_pool,
              submitExtrinsic(TransactionSource::External, *extrinsic))
      .WillOnce(Return(outcome::success()));
  EXPECT_OUTCOME_SUCCESS(
      hash,
      author_api->submitExtrinsic(TransactionSource::External, *extrinsic));
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
  EXPECT_CALL(*transaction_pool, submitExtrinsic(_, _))
      .WillOnce(Return(outcome::failure(DummyError::ERROR)));
  EXPECT_OUTCOME_ERROR(
      res,
      author_api->submitExtrinsic(TransactionSource::External, *extrinsic),
      DummyError::ERROR);
}

MATCHER_P(eventsAreEqual, n, "") {
  return (arg.id == n.id) and (arg.type == n.type);
}

/**
 * @given unsupported KeyTypeId for author_insertKey RPC call
 * @when insertKey called, check on supported key types fails
 * @then corresponing error is returned
 */
TEST_F(AuthorApiTest, InsertKeyUnsupported) {
  EXPECT_OUTCOME_ERROR(
      res,
      author_api->insertKey(decodeKeyTypeIdFromStr("dumy"), {}, {}),
      CryptoStoreError::UNSUPPORTED_KEY_TYPE);
}

/**
 * @given babe key type with seed and public key
 * @when insertKey called, check on key of the key type existence succeeds
 * @then corresponding error is returned
 */
TEST_F(AuthorApiTest, InsertKeyBabeAlreadyExists) {
  EXPECT_CALL(*store, getSr25519PublicKeys(KEY_TYPE_BABE))
      .Times(1)
      .WillOnce(Return(CryptoStore::Sr25519Keys{Sr25519PublicKey{}}));
  EXPECT_CALL(*store, findSr25519Keypair(KEY_TYPE_BABE, _))
      .Times(1)
      .WillOnce(Return(Sr25519Keypair{}));
  EXPECT_OUTCOME_ERROR(res,
                       author_api->insertKey(KEY_TYPE_BABE, {}, {}),
                       CryptoStoreError::BABE_ALREADY_EXIST);
}

/**
 * @given gran key type with seed and public key
 * @when insertKey called, check on key of the key type existence succeeds
 * @then corresponding error is returned
 */
TEST_F(AuthorApiTest, InsertKeyGranAlreadyExists) {
  EXPECT_CALL(*store, getEd25519PublicKeys(KEY_TYPE_GRAN))
      .Times(1)
      .WillOnce(Return(CryptoStore::Ed25519Keys{Ed25519PublicKey{}}));
  EXPECT_CALL(*store, findEd25519Keypair(KEY_TYPE_GRAN, _))
      .Times(1)
      .WillOnce(Return(Ed25519Keypair{}));
  EXPECT_OUTCOME_ERROR(res,
                       author_api->insertKey(KEY_TYPE_GRAN, {}, {}),
                       CryptoStoreError::GRAN_ALREADY_EXIST);
}

/**
 * @given babe key type with seed and public key
 * @when insertKey called, all checks passed
 * @then call succeeds
 */
TEST_F(AuthorApiTest, InsertKeyBabe) {
  EXPECT_CALL(*store, getEd25519PublicKeys(KEY_TYPE_GRAN))
      .Times(1)
      .WillRepeatedly(Return(CryptoStore::Ed25519Keys{}));
  EXPECT_CALL(*store, getSr25519PublicKeys(KEY_TYPE_AUDI))
      .Times(1)
      .WillRepeatedly(Return(CryptoStore::Sr25519Keys{}));
  EXPECT_CALL(*store, getSr25519PublicKeys(KEY_TYPE_BABE))
      .Times(2)
      .WillRepeatedly(Return(CryptoStore::Sr25519Keys{}));
  Sr25519Seed seed;
  Sr25519PublicKey public_key;
  EXPECT_CALL(*store, generateSr25519Keypair(KEY_TYPE_BABE, seed))
      .WillOnce(Return(Sr25519Keypair{{}, public_key}));
  EXPECT_OUTCOME_SUCCESS(
      res, author_api->insertKey(KEY_TYPE_BABE, seed, public_key));
}

/**
 * @given authority discovery key type with seed and public key
 * @when insertKey called, all checks passed
 * @then call succeeds
 */
TEST_F(AuthorApiTest, InsertKeyAudi) {
  EXPECT_CALL(*store, getEd25519PublicKeys(KEY_TYPE_GRAN))
      .Times(1)
      .WillRepeatedly(Return(CryptoStore::Ed25519Keys{}));
  EXPECT_CALL(*store, getSr25519PublicKeys(KEY_TYPE_AUDI))
      .Times(2)
      .WillRepeatedly(Return(CryptoStore::Sr25519Keys{}));
  EXPECT_CALL(*store, getSr25519PublicKeys(KEY_TYPE_BABE))
      .Times(1)
      .WillRepeatedly(Return(CryptoStore::Sr25519Keys{}));
  Sr25519Seed seed;
  Sr25519PublicKey public_key;
  EXPECT_CALL(*store, generateSr25519Keypair(KEY_TYPE_AUDI, seed))
      .WillOnce(Return(Sr25519Keypair{{}, public_key}));
  EXPECT_OUTCOME_SUCCESS(
      res, author_api->insertKey(KEY_TYPE_AUDI, seed, public_key));
}

/**
 * @given gran key type with seed and public key
 * @when insertKey called, all checks passed
 * @then call succeeds
 */
TEST_F(AuthorApiTest, InsertKeyGran) {
  EXPECT_CALL(*store, getEd25519PublicKeys(KEY_TYPE_GRAN))
      .Times(2)
      .WillRepeatedly(Return(CryptoStore::Ed25519Keys{}));
  EXPECT_CALL(*store, getSr25519PublicKeys(KEY_TYPE_AUDI))
      .Times(1)
      .WillRepeatedly(Return(CryptoStore::Sr25519Keys{}));
  EXPECT_CALL(*store, getSr25519PublicKeys(KEY_TYPE_BABE))
      .Times(1)
      .WillRepeatedly(Return(CryptoStore::Sr25519Keys{}));
  Ed25519Seed seed;
  Ed25519PublicKey public_key;
  EXPECT_CALL(*store, generateEd25519Keypair(KEY_TYPE_GRAN, seed))
      .WillOnce(Return(Ed25519Keypair{{}, public_key}));
  EXPECT_OUTCOME_SUCCESS(
      res, author_api->insertKey(KEY_TYPE_GRAN, seed, public_key));
}

/**
 * @given empty keys sequence
 * @when hasSessionKeys called
 * @then call succeeds, false result
 * NOTE could be special error
 */
TEST_F(AuthorApiTest, HasSessionKeysEmpty) {
  Buffer keys;
  EXPECT_OUTCOME_SUCCESS(
      res,
      author_api->hasSessionKeys(gsl::make_span(keys.data(), keys.size())));
  EXPECT_EQ(res.value(), false);
}

/**
 * @given keys sequence less than 1 key
 * @when hasSessionKeys called
 * @then call succeeds, false result
 * NOTE could be special error
 */
TEST_F(AuthorApiTest, HasSessionKeysLessThanOne) {
  Buffer keys;
  keys.resize(31);
  EXPECT_OUTCOME_SUCCESS(
      res,
      author_api->hasSessionKeys(gsl::make_span(keys.data(), keys.size())));
  EXPECT_EQ(res.value(), false);
}

/**
 * @given keys sequence greater than 6 keys
 * @when hasSessionKeys called
 * @then call succeeds, false result
 * NOTE could be special error
 */
TEST_F(AuthorApiTest, HasSessionKeysOverload) {
  Buffer keys;
  keys.resize(32 * 6 + 1);
  EXPECT_OUTCOME_SUCCESS(
      res,
      author_api->hasSessionKeys(gsl::make_span(keys.data(), keys.size())));
  EXPECT_EQ(res.value(), false);
}

/**
 * @given keys sequence not equal to n*32 in size
 * @when hasSessionKeys called
 * @then call succeeds, false result
 * NOTE could be special error
 */
TEST_F(AuthorApiTest, HasSessionKeysNotEqualKeys) {
  Buffer keys;
  keys.resize(32 * 5 + 1);
  EXPECT_OUTCOME_SUCCESS(
      res,
      author_api->hasSessionKeys(gsl::make_span(keys.data(), keys.size())));
  EXPECT_EQ(res.value(), false);
}

/**
 * @given keys sequence of 6 keys
 * @when hasSessionKeys called, all keys found
 * @then call succeeds, true result
 */
TEST_F(AuthorApiTest, HasSessionKeysSuccess6Keys) {
  Buffer keys;
  keys.resize(32 * 6);
  outcome::result<Ed25519Keypair> edOk = Ed25519Keypair{};
  outcome::result<Sr25519Keypair> srOk = Sr25519Keypair{};
  InSequence s;
  EXPECT_CALL(*store, findEd25519Keypair(KEY_TYPE_GRAN, _))
      .Times(1)
      .WillOnce(Return(edOk));
  EXPECT_CALL(*store, findSr25519Keypair(KEY_TYPE_BABE, _))
      .Times(1)
      .WillOnce(Return(srOk));
  EXPECT_CALL(*store, findSr25519Keypair(KEY_TYPE_IMON, _))
      .Times(1)
      .WillOnce(Return(srOk));
  EXPECT_CALL(*store, findSr25519Keypair(KEY_TYPE_PARA, _))
      .Times(1)
      .WillOnce(Return(srOk));
  EXPECT_CALL(*store, findSr25519Keypair(KEY_TYPE_ASGN, _))
      .Times(1)
      .WillOnce(Return(srOk));
  EXPECT_CALL(*store, findSr25519Keypair(KEY_TYPE_AUDI, _))
      .Times(1)
      .WillOnce(Return(srOk));
  EXPECT_OUTCOME_SUCCESS(
      res,
      author_api->hasSessionKeys(gsl::make_span(keys.data(), keys.size())));
  EXPECT_EQ(res.value(), true);
}

/**
 * @given keys sequence of 1 key
 * @when hasSessionKeys called, all keys found
 * @then call succeeds, true result
 */
TEST_F(AuthorApiTest, HasSessionKeysSuccess1Keys) {
  Buffer keys;
  keys.resize(32);
  outcome::result<Ed25519Keypair> edOk = Ed25519Keypair{};
  EXPECT_CALL(*store, findEd25519Keypair(KEY_TYPE_GRAN, _))
      .Times(1)
      .WillOnce(Return(edOk));
  EXPECT_OUTCOME_SUCCESS(
      res,
      author_api->hasSessionKeys(gsl::make_span(keys.data(), keys.size())));
  EXPECT_EQ(res.value(), true);
}

/**
 * @given keys sequence of 6 keys
 * @when hasSessionKeys called, 1 key not found
 * @then call succeeds, false result
 */
TEST_F(AuthorApiTest, HasSessionKeysFailureNotFound) {
  Buffer keys;
  keys.resize(32 * 6);
  outcome::result<Ed25519Keypair> edOk = Ed25519Keypair{};
  outcome::result<Sr25519Keypair> srErr = CryptoStoreError::KEY_NOT_FOUND;
  EXPECT_CALL(*store, findEd25519Keypair(_, _)).Times(1).WillOnce(Return(edOk));
  EXPECT_CALL(*store, findSr25519Keypair(_, _))
      .Times(1)
      .WillOnce(Return(srErr));
  EXPECT_OUTCOME_SUCCESS(
      res,
      author_api->hasSessionKeys(gsl::make_span(keys.data(), keys.size())));
  EXPECT_EQ(res.value(), false);
}

/**
 * @given pub_key and type
 * @when hasKey called, 1 key found
 * @then call succeeds, true result
 */
TEST_F(AuthorApiTest, HasKeySuccess) {
  EXPECT_OUTCOME_SUCCESS(
      res,
      author_api->hasKey(gsl::make_span(key_pair.public_key.data(), 32),
                         KEY_TYPE_BABE));
  EXPECT_EQ(res.value(), true);
}

/**
 * @given pub_key and type
 * @when hasKey called, key not found
 * @then call succeeds, false result
 */
TEST_F(AuthorApiTest, HasKeyFail) {
  EXPECT_OUTCOME_SUCCESS(res, author_api->hasKey({}, KEY_TYPE_BABE));
  EXPECT_EQ(res.value(), false);
}

/**
 * @given an extrinsic
 * @when submitting it through author api
 * @then it is successfully submitted, passed to the transaction pool and
 * propagated via gossiper, with corresponding events catched
 */
TEST_F(AuthorApiTest, SubmitAndWatchExtrinsicSubmitsAndWatches) {
  Transaction::Hash tx_hash{};

  EXPECT_CALL(*transaction_pool,
              constructTransaction(TransactionSource::External, *extrinsic))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*transaction_pool,
              submitExtrinsic(TransactionSource::External, *extrinsic))
      .WillOnce(testing::DoAll(
          testing::Invoke([this] {
            sub_engine->notify(ext_id,
                               ExtrinsicLifecycleEvent::Broadcast(ext_id, {}));
            sub_engine->notify(ext_id, ExtrinsicLifecycleEvent::Future(ext_id));
            sub_engine->notify(ext_id, ExtrinsicLifecycleEvent::Ready(ext_id));
          }),
          Return(outcome::success())));

  {
    testing::InSequence s;
    EXPECT_CALL(*event_receiver,
                receive(sub_id,
                        _,
                        ext_id,
                        eventsAreEqual(
                            ExtrinsicLifecycleEvent::Broadcast(ext_id, {}))));
    EXPECT_CALL(
        *event_receiver,
        receive(sub_id,
                _,
                ext_id,
                eventsAreEqual(ExtrinsicLifecycleEvent::Future(ext_id))));
    EXPECT_CALL(
        *event_receiver,
        receive(sub_id,
                _,
                ext_id,
                eventsAreEqual(ExtrinsicLifecycleEvent::Ready(ext_id))));
  }

  EXPECT_CALL(*api_service_mock, subscribeForExtrinsicLifecycle(tx_hash))
      .WillOnce(Return(sub_id));

  // throws because api service is uninitialized
  ASSERT_OUTCOME_SUCCESS(ret_sub_id,
                         author_api->submitAndWatchExtrinsic(*extrinsic));
  ASSERT_EQ(sub_id, ret_sub_id);
}

/**
 * @when requesting list of extrinsics
 * @then extrinsics are fetched from transaction pool and returned as a vector
 */
TEST_F(AuthorApiTest, PendingExtrinsics) {
  std::unordered_map<Transaction::Hash, std::shared_ptr<Transaction>> trxs;
  std::vector<Extrinsic> expected_result;

  EXPECT_CALL(*transaction_pool, getPendingTransactions())
      .WillOnce(ReturnRef(trxs));

  ASSERT_OUTCOME_SUCCESS(actual_result, author_api->pendingExtrinsics());
  ASSERT_EQ(expected_result, actual_result);
}

/**
 * @given subscription id
 * @when requesting to unwatch extrinsic
 * @then request is forwarded to api service, result returned
 */
TEST_F(AuthorApiTest, UnwatchExtrinsic) {
  kagome::primitives::SubscriptionId sub_id = 0;

  EXPECT_CALL(*api_service_mock, unsubscribeFromExtrinsicLifecycle(sub_id))
      .WillOnce(Return(true));

  ASSERT_TRUE(author_api->unwatchExtrinsic(sub_id));
}
