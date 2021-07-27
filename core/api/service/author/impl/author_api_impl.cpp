/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/author/impl/author_api_impl.hpp"

#include <jsonrpc-lean/fault.h>
#include <boost/assert.hpp>
#include <boost/system/error_code.hpp>
#include <stdexcept>

#include "api/service/api_service.hpp"
#include "common/visitor.hpp"
#include "crypto/crypto_store.hpp"
#include "crypto/crypto_store/crypto_store_impl.hpp"
#include "crypto/crypto_store/crypto_suites.hpp"
#include "crypto/crypto_store/key_file_storage.hpp"
#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/hasher.hpp"
#include "network/transactions_transmitter.hpp"
#include "primitives/transaction.hpp"
#include "runtime/tagged_transaction_queue.hpp"
#include "scale/scale_decoder_stream.hpp"
#include "subscription/subscriber.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::api {
  AuthorApiImpl::AuthorApiImpl(
      sptr<runtime::TaggedTransactionQueue> api,
      sptr<transaction_pool::TransactionPool> pool,
      sptr<crypto::Hasher> hasher,
      sptr<network::TransactionsTransmitter> transactions_transmitter,
      sptr<crypto::CryptoStore> store,
      sptr<crypto::SessionKeys> keys,
      sptr<crypto::KeyFileStorage> key_store)
      : api_{std::move(api)},
        pool_{std::move(pool)},
        hasher_{std::move(hasher)},
        transactions_transmitter_{std::move(transactions_transmitter)},
        store_{std::move(store)},
        keys_{std::move(keys)},
        key_store_{std::move(key_store)},
        last_id_{0},
        logger_{log::createLogger("AuthorApi", "author_api")} {
    BOOST_ASSERT_MSG(api_ != nullptr, "author api is nullptr");
    BOOST_ASSERT_MSG(pool_ != nullptr, "transaction pool is nullptr");
    BOOST_ASSERT_MSG(hasher_ != nullptr, "hasher is nullptr");
    BOOST_ASSERT_MSG(transactions_transmitter_ != nullptr,
                     "transactions_transmitter is nullptr");
    BOOST_ASSERT_MSG(store_ != nullptr, "crypto store is nullptr");
    BOOST_ASSERT_MSG(keys_ != nullptr, "session keys store is nullptr");
    BOOST_ASSERT_MSG(key_store_ != nullptr, "key store is nullptr");
    BOOST_ASSERT_MSG(logger_ != nullptr, "logger is nullptr");
  }

  void AuthorApiImpl::setApiService(
      std::shared_ptr<api::ApiService> const &api_service) {
    BOOST_ASSERT(api_service != nullptr);
    api_service_ = api_service;
  }

  outcome::result<common::Hash256> AuthorApiImpl::submitExtrinsic(
      const primitives::Extrinsic &extrinsic) {
    OUTCOME_TRY(tx, constructTransaction(extrinsic, boost::none));

    if (tx.should_propagate) {
      transactions_transmitter_->propagateTransactions(
          gsl::make_span(std::vector{tx}));
    }
    auto hash = tx.hash;
    // send to pool
    OUTCOME_TRY(pool_->submitOne(std::move(tx)));
    return hash;
  }

  outcome::result<void> AuthorApiImpl::insertKey(
      crypto::KeyTypeId key_type,
      const gsl::span<const uint8_t> &seed,
      const gsl::span<const uint8_t> &public_key) {
    if (not(crypto::KEY_TYPE_BABE == key_type)
        && not(crypto::KEY_TYPE_GRAN == key_type)) {
      SL_INFO(logger_, "Unsupported key type, only BABE and GRAN are accepted");
      return outcome::failure(crypto::CryptoStoreError::UNSUPPORTED_KEY_TYPE);
    };
    if (crypto::KEY_TYPE_BABE == key_type && keys_->getBabeKeyPair()) {
      SL_INFO(logger_, "Babe key already exists and won't be replaced");
      return outcome::failure(crypto::CryptoStoreError::BABE_ALREADY_EXIST);
    }
    if (crypto::KEY_TYPE_GRAN == key_type && keys_->getGranKeyPair()) {
      SL_INFO(logger_, "Grandpa key already exists and won't be replaced");
      return outcome::failure(crypto::CryptoStoreError::GRAN_ALREADY_EXIST);
    }
    auto res = key_store_->saveKeyPair(key_type, public_key, seed);
    // explicitly load keys from store to cache
    keys_->getBabeKeyPair();
    keys_->getGranKeyPair();
    return res;
  }

  // logic here is polkadot specific only!
  // it could be extended by reading config from chainspec palletSession/keys
  // value
  outcome::result<bool> AuthorApiImpl::hasSessionKeys(
      const gsl::span<const uint8_t> &keys) {
    scale::ScaleDecoderStream stream(keys);
    std::array<uint8_t, 32> key;
    if (keys.size() < 32 || keys.size() > 32 * 6 || (keys.size() % 32) != 0) {
      SL_WARN(logger_,
              "not valid key sequence, author_hasSessionKeys RPC call expects "
              "no more than 6 public keys in concatenated string, keys should "
              "be 32 byte in size");
      return false;
    }
    stream >> key;
    if (store_->findEd25519Keypair(
            crypto::KEY_TYPE_GRAN,
            crypto::Ed25519PublicKey(common::Blob<32>(key)))) {
      unsigned count = 1;
      while (stream.currentIndex() < keys.size()) {
        stream >> key;
        if (not store_->findSr25519Keypair(
                crypto::polkadot_key_order[count++],
                crypto::Sr25519PublicKey(common::Blob<32>(key)))) {
          return false;
        }
      }
      return true;
    }
    return false;
  }

  outcome::result<bool> AuthorApiImpl::hasKey(
      const gsl::span<const uint8_t> &public_key, crypto::KeyTypeId key_type) {
    auto res = key_store_->searchForSeed(key_type, public_key);
    if (not res)
      return res.error();
    else
      return res.value() ? true : false;
  }

  outcome::result<std::vector<primitives::Extrinsic>>
  AuthorApiImpl::pendingExtrinsics() {
    auto &pending_txs = pool_->getPendingTransactions();

    std::vector<primitives::Extrinsic> result;
    result.reserve(pending_txs.size());

    std::transform(pending_txs.begin(),
                   pending_txs.end(),
                   std::back_inserter(result),
                   [](auto &it) { return it.second->ext; });

    return result;
  }

  outcome::result<std::vector<primitives::Extrinsic>>
  AuthorApiImpl::removeExtrinsic(
      const std::vector<primitives::ExtrinsicKey> &keys) {
    BOOST_ASSERT_MSG(false, "not implemented");  // NOLINT
    return outcome::failure(boost::system::error_code{});
  }

  outcome::result<AuthorApi::SubscriptionId>
  AuthorApiImpl::submitAndWatchExtrinsic(Extrinsic extrinsic) {
    OUTCOME_TRY(tx, constructTransaction(extrinsic, last_id_++));

    SubscriptionId sub_id{};
    if (auto service = api_service_.lock()) {
      OUTCOME_TRY(sub_id_, service->subscribeForExtrinsicLifecycle(tx));
      sub_id = sub_id_;
    } else {
      throw jsonrpc::InternalErrorFault(
          "Internal error. Api service not initialized.");
    }
    if (tx.should_propagate) {
      transactions_transmitter_->propagateTransactions(
          gsl::make_span(std::vector{tx}));
    }

    // send to pool
    OUTCOME_TRY(pool_->submitOne(std::move(tx)));

    return sub_id;
  }

  outcome::result<bool> AuthorApiImpl::unwatchExtrinsic(SubscriptionId sub_id) {
    if (auto service = api_service_.lock()) {
      return service->unsubscribeFromExtrinsicLifecycle(sub_id);
    }
    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

  outcome::result<primitives::Transaction> AuthorApiImpl::constructTransaction(
      primitives::Extrinsic extrinsic,
      boost::optional<primitives::Transaction::ObservedId> id) const {
    OUTCOME_TRY(res,
                api_->validate_transaction(
                    primitives::TransactionSource::External, extrinsic));

    return visit_in_place(
        res,
        [&](const primitives::TransactionValidityError &e) {
          return visit_in_place(
              e,
              // return either invalid or unknown validity error
              [](const auto &validity_error)
                  -> outcome::result<primitives::Transaction> {
                return validity_error;
              });
        },
        [&](const primitives::ValidTransaction &v)
            -> outcome::result<primitives::Transaction> {
          common::Hash256 hash = hasher_->blake2b_256(extrinsic.data);
          size_t length = extrinsic.data.size();

          return primitives::Transaction{id,
                                         extrinsic,
                                         length,
                                         hash,
                                         v.priority,
                                         v.longevity,
                                         v.requires,
                                         v.provides,
                                         v.propagate};
        });
  }

}  // namespace kagome::api
