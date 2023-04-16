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
#include "blockchain/block_tree.hpp"
#include "crypto/crypto_store.hpp"
#include "crypto/crypto_store/crypto_store_impl.hpp"
#include "crypto/crypto_store/crypto_suites.hpp"
#include "crypto/crypto_store/key_file_storage.hpp"
#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/hasher.hpp"
#include "primitives/transaction.hpp"
#include "runtime/runtime_api/session_keys_api.hpp"
#include "scale/scale_decoder_stream.hpp"
#include "subscription/subscriber.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::api {
  const std::vector<crypto::KnownKeyTypeId> kKeyTypes{
      crypto::KEY_TYPE_BABE,
      crypto::KEY_TYPE_GRAN,
      crypto::KEY_TYPE_AUDI,
  };

  AuthorApiImpl::AuthorApiImpl(sptr<runtime::SessionKeysApi> key_api,
                               sptr<transaction_pool::TransactionPool> pool,
                               sptr<crypto::CryptoStore> store,
                               sptr<crypto::SessionKeys> keys,
                               sptr<crypto::KeyFileStorage> key_store,
                               sptr<blockchain::BlockTree> block_tree)
      : keys_api_(std::move(key_api)),
        pool_{std::move(pool)},
        store_{std::move(store)},
        keys_{std::move(keys)},
        key_store_{std::move(key_store)},
        block_tree_{std::move(block_tree)},
        logger_{log::createLogger("AuthorApi", "author_api")} {
    BOOST_ASSERT_MSG(keys_api_ != nullptr, "session keys api is nullptr");
    BOOST_ASSERT_MSG(pool_ != nullptr, "transaction pool is nullptr");
    BOOST_ASSERT_MSG(store_ != nullptr, "crypto store is nullptr");
    BOOST_ASSERT_MSG(keys_ != nullptr, "session keys store is nullptr");
    BOOST_ASSERT_MSG(key_store_ != nullptr, "key store is nullptr");
    BOOST_ASSERT_MSG(block_tree_ != nullptr, "block tree is nullptr");
    BOOST_ASSERT_MSG(logger_ != nullptr, "logger is nullptr");
  }

  void AuthorApiImpl::setApiService(
      std::shared_ptr<api::ApiService> const &api_service) {
    BOOST_ASSERT(api_service != nullptr);
    api_service_ = api_service;
  }

  outcome::result<common::Hash256> AuthorApiImpl::submitExtrinsic(
      primitives::TransactionSource source,
      const primitives::Extrinsic &extrinsic) {
    return pool_->submitExtrinsic(source, extrinsic);
  }

  outcome::result<void> AuthorApiImpl::insertKey(
      crypto::KeyTypeId key_type,
      const gsl::span<const uint8_t> &seed,
      const gsl::span<const uint8_t> &public_key) {
    if (std::find(kKeyTypes.begin(), kKeyTypes.end(), key_type)
        == kKeyTypes.end()) {
      std::string types;
      for (auto &type : kKeyTypes) {
        types.append(encodeKeyTypeIdToStr(type));
        types.push_back(' ');
      }
      types.pop_back();
      SL_INFO(logger_, "Unsupported key type, only [{}] are accepted", types);
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
    if (crypto::KEY_TYPE_AUDI == key_type && keys_->getAudiKeyPair()) {
      SL_INFO(logger_,
              "Authority discovery key already exists and won't be replaced");
      return outcome::failure(crypto::CryptoStoreError::AUDI_ALREADY_EXIST);
    }
    if (crypto::KEY_TYPE_BABE == key_type
        or crypto::KEY_TYPE_AUDI == key_type) {
      OUTCOME_TRY(seed_typed, crypto::Sr25519Seed::fromSpan(seed));
      OUTCOME_TRY(public_key_typed,
                  crypto::Sr25519PublicKey::fromSpan(public_key));
      OUTCOME_TRY(keypair,
                  store_->generateSr25519Keypair(key_type, seed_typed));
      if (public_key_typed != keypair.public_key) {
        return outcome::failure(crypto::CryptoStoreError::WRONG_PUBLIC_KEY);
      }
    }
    if (crypto::KEY_TYPE_GRAN == key_type) {
      OUTCOME_TRY(seed_typed, crypto::Ed25519Seed::fromSpan(seed));
      OUTCOME_TRY(public_key_typed,
                  crypto::Ed25519PublicKey::fromSpan(public_key));
      OUTCOME_TRY(
          keypair,
          store_->generateEd25519Keypair(crypto::KEY_TYPE_GRAN, seed_typed));
      if (public_key_typed != keypair.public_key) {
        return outcome::failure(crypto::CryptoStoreError::WRONG_PUBLIC_KEY);
      }
    }
    auto res = key_store_->saveKeyPair(key_type, public_key, seed);
    // explicitly load keys from store to cache
    keys_->getBabeKeyPair();
    keys_->getGranKeyPair();
    keys_->getAudiKeyPair();
    return res;
  }

  outcome::result<common::Buffer> AuthorApiImpl::rotateKeys() {
    OUTCOME_TRY(encoded_session_keys,
                keys_api_->generate_session_keys(block_tree_->bestLeaf().hash,
                                                 std::nullopt));
    return std::move(encoded_session_keys);
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
    return outcome::failure(std::errc::not_supported);
  }

  outcome::result<AuthorApi::SubscriptionId>
  AuthorApiImpl::submitAndWatchExtrinsic(Extrinsic extrinsic) {
    if (auto service = api_service_.lock()) {
      OUTCOME_TRY(
          tx,
          pool_->constructTransaction(TransactionSource::External, extrinsic));
      OUTCOME_TRY(sub_id, service->subscribeForExtrinsicLifecycle(tx.hash));
      OUTCOME_TRY(tx_hash,
                  submitExtrinsic(
                      // submit and watch could be executed only
                      // from RPC call, so External source is chosen
                      TransactionSource::External,
                      extrinsic));
      BOOST_ASSERT(tx_hash == tx.hash);

      SL_DEBUG(logger_, "Submit and watch transaction with hash {}", tx_hash);

      return sub_id;
    }

    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

  outcome::result<bool> AuthorApiImpl::unwatchExtrinsic(SubscriptionId sub_id) {
    if (auto service = api_service_.lock()) {
      return service->unsubscribeFromExtrinsicLifecycle(sub_id);
    }
    throw jsonrpc::InternalErrorFault(
        "Internal error. Api service not initialized.");
  }

}  // namespace kagome::api
