/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/crypto_store_impl.hpp"

#include <fstream>

#include <gsl/span>

#include "common/visitor.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, CryptoStoreError, e) {
  using E = kagome::crypto::CryptoStoreError;
  switch (e) {
    case E::UNSUPPORTED_KEY_TYPE:
      return "key type is not supported";
    case E::UNSUPPORTED_CRYPTO_TYPE:
      return "cryptographic type is not supported";
    case E::WRONG_SEED_SIZE:
      return "wrong seed size";
    case E::KEY_NOT_FOUND:
      return "key not found";
  }
  return "Unknown CryptoStoreError code";
}

namespace kagome::crypto {

  CryptoStoreImpl::CryptoStoreImpl(
      std::shared_ptr<Ed25519Suite> ed_suite,
      std::shared_ptr<Sr25519Suite> sr_suite,
      std::shared_ptr<Bip39Provider> bip39_provider,
      std::shared_ptr<KeyFileStorage> key_fs)
      : file_storage_{std::move(key_fs)},
        ed_suite_{std::move(ed_suite)},
        sr_suite_{std::move(sr_suite)},
        bip39_provider_{std::move(bip39_provider)},
        logger_{common::createLogger("CryptoStore")} {
    BOOST_ASSERT(ed_suite_ != nullptr);
    BOOST_ASSERT(sr_suite_ != nullptr);
    BOOST_ASSERT(bip39_provider_ != nullptr);
    BOOST_ASSERT(file_storage_ != nullptr);
  }

  outcome::result<Ed25519Keypair> CryptoStoreImpl::generateEd25519Keypair(
      KeyTypeId key_type, std::string_view mnemonic_phrase) {
    OUTCOME_TRY(kp, generateKeypair(mnemonic_phrase, *ed_suite_));
    getCache(ed_suite_, ed_caches_, key_type)
        .insert(kp.public_key, kp.secret_key);
    return std::move(kp);
  }

  outcome::result<Sr25519Keypair> CryptoStoreImpl::generateSr25519Keypair(
      KeyTypeId key_type, std::string_view mnemonic_phrase) {
    OUTCOME_TRY(kp, generateKeypair(mnemonic_phrase, *sr_suite_));
    getCache(sr_suite_, sr_caches_, key_type)
        .insert(kp.public_key, kp.secret_key);
    return std::move(kp);
  }

  Ed25519Keypair CryptoStoreImpl::generateEd25519Keypair(
      KeyTypeId key_type, const Ed25519Seed &seed) {
    auto kp = ed_suite_->generateKeypair(seed);
    getCache(ed_suite_, ed_caches_, key_type)
        .insert(kp.public_key, kp.secret_key);
    return kp;
  }

  Sr25519Keypair CryptoStoreImpl::generateSr25519Keypair(
      KeyTypeId key_type, const Sr25519Seed &seed) {
    auto kp = sr_suite_->generateKeypair(seed);
    getCache(sr_suite_, sr_caches_, key_type)
        .insert(kp.public_key, kp.secret_key);
    return kp;
  }

  outcome::result<Ed25519Keypair> CryptoStoreImpl::generateEd25519KeypairOnDisk(
      KeyTypeId key_type) {
    auto kp = ed_suite_->generateRandomKeypair();
    getCache(ed_suite_, ed_caches_, key_type)
        .insert(kp.public_key, kp.secret_key);
    OUTCOME_TRY(
        file_storage_->saveKeypair(key_type, kp.public_key, kp.secret_key));
    return kp;
  }

  outcome::result<Sr25519Keypair> CryptoStoreImpl::generateSr25519KeypairOnDisk(
      KeyTypeId key_type) {
    auto kp = sr_suite_->generateRandomKeypair();
    getCache(sr_suite_, sr_caches_, key_type)
        .insert(kp.public_key, kp.secret_key);
    OUTCOME_TRY(
        file_storage_->saveKeypair(key_type, kp.public_key, kp.secret_key));
    return kp;
  }

  outcome::result<Ed25519Keypair> CryptoStoreImpl::findEd25519Keypair(
      KeyTypeId key_type, const Ed25519PublicKey &pk) const {
    auto kp_opt = getCache(ed_suite_, ed_caches_, key_type).searchKeypair(pk);
    if (kp_opt) {
      return kp_opt.value();
    }
    OUTCOME_TRY(seed_bytes,
                file_storage_->searchForSeed(key_type, gsl::make_span(pk)));
    if (not seed_bytes) {
      return CryptoStoreError::KEY_NOT_FOUND;
    }
    OUTCOME_TRY(seed, Ed25519Seed::fromSpan(seed_bytes.value()));
    return ed_suite_->generateKeypair(seed);
  }

  outcome::result<Sr25519Keypair> CryptoStoreImpl::findSr25519Keypair(
      KeyTypeId key_type, const Sr25519PublicKey &pk) const {
    auto kp_opt = getCache(sr_suite_, sr_caches_, key_type).searchKeypair(pk);
    if (kp_opt) {
      return kp_opt.value();
    }
    OUTCOME_TRY(seed_bytes,
                file_storage_->searchForSeed(key_type, gsl::make_span(pk)));
    if (not seed_bytes) {
      return CryptoStoreError::KEY_NOT_FOUND;
    }
    OUTCOME_TRY(seed, Sr25519Seed::fromSpan(seed_bytes.value()));
    return sr_suite_->generateKeypair(seed);
  }

  outcome::result<CryptoStoreImpl::Ed25519Keys>
  CryptoStoreImpl::getEd25519PublicKeys(KeyTypeId key_type) const {
    return getPublicKeys(
        key_type, getCache(ed_suite_, ed_caches_, key_type), *ed_suite_);
  }

  outcome::result<CryptoStoreImpl::Sr25519Keys>
  CryptoStoreImpl::getSr25519PublicKeys(KeyTypeId key_type) const {
    return getPublicKeys(
        key_type, getCache(sr_suite_, sr_caches_, key_type), *sr_suite_);
  }

  boost::optional<Ed25519Keypair> CryptoStoreImpl::getGrandpaKeypair() const {
    auto keys = getEd25519PublicKeys(KEY_TYPE_GRAN);
    if (not keys or keys.value().empty()) {
      return boost::none;
    }
    auto kp = findEd25519Keypair(KEY_TYPE_GRAN, keys.value().at(0));
    return kp ? boost::make_optional(kp.value()) : boost::none;
  }

  boost::optional<Sr25519Keypair> CryptoStoreImpl::getBabeKeypair() const {
    auto keys = getSr25519PublicKeys(KEY_TYPE_BABE);
    if (not keys or keys.value().empty()) {
      return boost::none;
    }
    auto kp = findSr25519Keypair(KEY_TYPE_BABE, keys.value().at(0));
    return kp ? boost::make_optional(kp.value()) : boost::none;
  }

  boost::optional<libp2p::crypto::KeyPair> CryptoStoreImpl::getLibp2pKeypair()
      const {
    auto keys = getEd25519PublicKeys(KEY_TYPE_LP2P);
    if (not keys or keys.value().empty()) {
      return boost::none;
    }
    auto kp = findEd25519Keypair(KEY_TYPE_LP2P, keys.value().at(0));
    if (kp) {
      auto &secret_key = kp.value().secret_key;
      auto &public_key = kp.value().public_key;
      libp2p::crypto::PublicKey lp2p_public{
          {libp2p::crypto::Key::Type::Ed25519,
           std::vector<uint8_t>{public_key.cbegin(), public_key.cend()}}};
      libp2p::crypto::PrivateKey lp2p_private{
          {libp2p::crypto::Key::Type::Ed25519,
           std::vector<uint8_t>{secret_key.cbegin(), secret_key.cend()}}};
      return libp2p::crypto::KeyPair{
          .publicKey = lp2p_public,
          .privateKey = lp2p_private,
      };
    }
    return boost::none;
  }
}  // namespace kagome::crypto
