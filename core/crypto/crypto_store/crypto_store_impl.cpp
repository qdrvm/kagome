/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/crypto_store_impl.hpp"

#include <gsl/span>

#include "common/bytestr.hpp"
#include "common/visitor.hpp"
#include "utils/read_file.hpp"

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
    case E::BABE_ALREADY_EXIST:
      return "BABE key already exists";
    case E::GRAN_ALREADY_EXIST:
      return "GRAN key already exists";
    case E::AUDI_ALREADY_EXIST:
      return "AUDI key already exists";
    case E::WRONG_PUBLIC_KEY:
      return "Public key doesn't match seed";
  }
  return "Unknown CryptoStoreError code";
}

namespace kagome::crypto {

  CryptoStoreImpl::CryptoStoreImpl(
      std::shared_ptr<EcdsaSuite> ecdsa_suite,
      std::shared_ptr<Ed25519Suite> ed_suite,
      std::shared_ptr<Sr25519Suite> sr_suite,
      std::shared_ptr<Bip39Provider> bip39_provider,
      std::shared_ptr<CSPRNG> csprng,
      std::shared_ptr<KeyFileStorage> key_fs)
      : file_storage_{std::move(key_fs)},
        ecdsa_suite_{std::move(ecdsa_suite)},
        ed_suite_{std::move(ed_suite)},
        sr_suite_{std::move(sr_suite)},
        bip39_provider_{std::move(bip39_provider)},
        csprng_{std::move(csprng)},
        logger_{log::createLogger("CryptoStore", "crypto_store")} {
    BOOST_ASSERT(ecdsa_suite_ != nullptr);
    BOOST_ASSERT(ed_suite_ != nullptr);
    BOOST_ASSERT(sr_suite_ != nullptr);
    BOOST_ASSERT(bip39_provider_ != nullptr);
    BOOST_ASSERT(file_storage_ != nullptr);
  }

  outcome::result<EcdsaKeypair> CryptoStoreImpl::generateEcdsaKeypair(
      KeyTypeId key_type, std::string_view mnemonic_phrase) {
    OUTCOME_TRY(kp, generateKeypair(mnemonic_phrase, *ecdsa_suite_));
    getCache(ecdsa_suite_, ecdsa_caches_, key_type)
        .insert(kp.public_key, kp.secret_key);
    return kp;
  }

  outcome::result<Ed25519Keypair> CryptoStoreImpl::generateEd25519Keypair(
      KeyTypeId key_type, std::string_view mnemonic_phrase) {
    OUTCOME_TRY(kp, generateKeypair(mnemonic_phrase, *ed_suite_));
    getCache(ed_suite_, ed_caches_, key_type)
        .insert(kp.public_key, kp.secret_key);
    return kp;
  }

  outcome::result<Sr25519Keypair> CryptoStoreImpl::generateSr25519Keypair(
      KeyTypeId key_type, std::string_view mnemonic_phrase) {
    OUTCOME_TRY(kp, generateKeypair(mnemonic_phrase, *sr_suite_));
    getCache(sr_suite_, sr_caches_, key_type)
        .insert(kp.public_key, kp.secret_key);
    return kp;
  }

  outcome::result<EcdsaKeypair> CryptoStoreImpl::generateEcdsaKeypair(
      KeyTypeId key_type, const EcdsaSeed &seed) {
    OUTCOME_TRY(kp, ecdsa_suite_->generateKeypair(seed, {}));
    getCache(ecdsa_suite_, ecdsa_caches_, key_type)
        .insert(kp.public_key, kp.secret_key);
    return kp;
  }

  outcome::result<Ed25519Keypair> CryptoStoreImpl::generateEd25519Keypair(
      KeyTypeId key_type, const Ed25519Seed &seed) {
    OUTCOME_TRY(kp, ed_suite_->generateKeypair(seed, {}));
    getCache(ed_suite_, ed_caches_, key_type)
        .insert(kp.public_key, kp.secret_key);
    return kp;
  }

  outcome::result<Sr25519Keypair> CryptoStoreImpl::generateSr25519Keypair(
      KeyTypeId key_type, const Sr25519Seed &seed) {
    OUTCOME_TRY(kp, sr_suite_->generateKeypair(seed, {}));
    getCache(sr_suite_, sr_caches_, key_type)
        .insert(kp.public_key, kp.secret_key);
    return kp;
  }

  outcome::result<EcdsaKeypair> CryptoStoreImpl::generateEcdsaKeypairOnDisk(
      KeyTypeId key_type) {
    return generateKeypairOnDisk(key_type, ecdsa_suite_, ecdsa_caches_);
  }

  outcome::result<Ed25519Keypair> CryptoStoreImpl::generateEd25519KeypairOnDisk(
      KeyTypeId key_type) {
    return generateKeypairOnDisk(key_type, ed_suite_, ed_caches_);
  }

  outcome::result<Sr25519Keypair> CryptoStoreImpl::generateSr25519KeypairOnDisk(
      KeyTypeId key_type) {
    return generateKeypairOnDisk(key_type, sr_suite_, sr_caches_);
  }

  outcome::result<EcdsaKeypair> CryptoStoreImpl::findEcdsaKeypair(
      KeyTypeId key_type, const EcdsaPublicKey &pk) const {
    auto kp_opt =
        getCache(ecdsa_suite_, ecdsa_caches_, key_type).searchKeypair(pk);
    if (kp_opt) {
      return kp_opt.value();
    }
    OUTCOME_TRY(phrase,
                file_storage_->searchForPhrase(key_type, gsl::make_span(pk)));
    if (not phrase) {
      return CryptoStoreError::KEY_NOT_FOUND;
    }
    OUTCOME_TRY(bip, bip39_provider_->generateSeed(*phrase));
    return ecdsa_suite_->generateKeypair(bip);
  }

  outcome::result<Ed25519Keypair> CryptoStoreImpl::findEd25519Keypair(
      KeyTypeId key_type, const Ed25519PublicKey &pk) const {
    auto kp_opt = getCache(ed_suite_, ed_caches_, key_type).searchKeypair(pk);
    if (kp_opt) {
      return kp_opt.value();
    }
    OUTCOME_TRY(phrase,
                file_storage_->searchForPhrase(key_type, gsl::make_span(pk)));
    if (not phrase) {
      return CryptoStoreError::KEY_NOT_FOUND;
    }
    OUTCOME_TRY(bip, bip39_provider_->generateSeed(*phrase));
    return ed_suite_->generateKeypair(bip);
  }

  outcome::result<Sr25519Keypair> CryptoStoreImpl::findSr25519Keypair(
      KeyTypeId key_type, const Sr25519PublicKey &pk) const {
    auto kp_opt = getCache(sr_suite_, sr_caches_, key_type).searchKeypair(pk);
    if (kp_opt) {
      return kp_opt.value();
    }
    OUTCOME_TRY(phrase,
                file_storage_->searchForPhrase(key_type, gsl::make_span(pk)));
    if (not phrase) {
      return CryptoStoreError::KEY_NOT_FOUND;
    }
    OUTCOME_TRY(bip, bip39_provider_->generateSeed(*phrase));
    return sr_suite_->generateKeypair(bip);
  }

  outcome::result<CryptoStoreImpl::EcdsaKeys>
  CryptoStoreImpl::getEcdsaPublicKeys(KeyTypeId key_type) const {
    return getPublicKeys(key_type,
                         getCache(ecdsa_suite_, ecdsa_caches_, key_type),
                         *ecdsa_suite_);
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

  outcome::result<libp2p::crypto::KeyPair> CryptoStoreImpl::loadLibp2pKeypair(
      const CryptoStore::Path &key_path) const {
    std::string contents;
    if (not readFile(contents, key_path.string())) {
      return CryptoStoreError::KEY_NOT_FOUND;
    }
    BOOST_ASSERT(ED25519_SEED_LENGTH == contents.size()
                 or 2 * ED25519_SEED_LENGTH == contents.size());  // hex
    Ed25519Seed seed;
    if (ED25519_SEED_LENGTH == contents.size()) {
      OUTCOME_TRY(_seed, Ed25519Seed::fromSpan(str2byte(contents)));
      seed = _seed;
    } else if (2 * ED25519_SEED_LENGTH == contents.size()) {  // hex-encoded
      OUTCOME_TRY(_seed, Ed25519Seed::fromHex(contents));
      seed = _seed;
    } else {
      return CryptoStoreError::UNSUPPORTED_CRYPTO_TYPE;
    }
    OUTCOME_TRY(kp, ed_suite_->generateKeypair(seed, {}));
    return ed25519KeyToLibp2pKeypair(kp);
  }

  libp2p::crypto::KeyPair ed25519KeyToLibp2pKeypair(const Ed25519Keypair &kp) {
    const auto &secret_key = kp.secret_key;
    const auto &public_key = kp.public_key;
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
}  // namespace kagome::crypto
